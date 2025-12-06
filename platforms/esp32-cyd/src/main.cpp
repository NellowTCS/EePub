#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <dirent.h>
#include <esp_err.h>
#include <esp_heap_caps.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/gpio.h>
#include <driver/sdspi_host.h>
#include <driver/spi_common.h>

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

#include "DisplaySink.h"
#include "EePub.h"
#include "hal/Hal.h"

// ST7789_SPI_HOST is 1 (SPI2_HOST/HSPI) or 2 (SPI3_HOST/VSPI)
#if ST7789_SPI_HOST == 1
static SPIClass tftSpi(HSPI);
#else
static SPIClass tftSpi(VSPI);
#endif

static Adafruit_ST7789 tft(&tftSpi, ST7789_SPI_CONFIG_CS, ST7789_SPI_CONFIG_DC, ST7789_DEV_CONFIG_RESET);

class St7789TextSink : public DisplaySink {
public:
    explicit St7789TextSink(Adafruit_ST7789& panel, std::uint16_t width, std::uint16_t height)
        : tft(panel), w(width), h(height) {}

    void beginFrame(std::size_t) override {
        cursorY = topMargin;
        tft.fillScreen(ST77XX_BLACK);
        tft.setTextWrap(false);
        tft.setTextSize(1);
    }

    void drawLine(std::size_t, const LayoutEngine::LayoutLine& line) override {
        if (cursorY >= h - lineHeight) {
            cursorY = topMargin;
            tft.fillScreen(ST77XX_BLACK);
        }
        tft.setCursor(sideMargin, cursorY);
        uint16_t color = line.heading ? headingColor : textColor;
        tft.setTextColor(color, ST77XX_BLACK);
        if (line.text.empty()) {
            cursorY += lineHeight;
            return;
        }
        tft.print(line.text.c_str());
        cursorY += lineHeight;
    }

    void endFrame() override {}

private:
    Adafruit_ST7789& tft;
    const std::uint16_t w;
    const std::uint16_t h;
    int cursorY = 0;
    static constexpr int topMargin = 8;
    static constexpr int sideMargin = 6;
    static constexpr int lineHeight = 14;
    static constexpr uint16_t headingColor = ST77XX_CYAN;
    static constexpr uint16_t textColor = ST77XX_WHITE;
};

static std::unique_ptr<St7789TextSink> gSink;
static EePub gEePub;
static bool gRenderPending = false;
static bool gSdMounted = false;
static std::string gCurrentBook;

constexpr const char* kMountPoint = "/sdcard";
constexpr gpio_num_t kNavButton = GPIO_NUM_0;

void halLog(EePubLogLevel level, const char* message, void*) {
    const char* tag = (level == EePubLogLevel::Error)
        ? "ERR"
        : (level == EePubLogLevel::Warn ? "WRN" : "INF");
    Serial.printf("[EePub][%s] %s\n", tag, message ? message : "");
}

std::size_t halHeapFree(void*) {
    return heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
}

std::uint64_t halMillisHook(void*) {
    return millis();
}

bool initDisplay() {
    pinMode(DISPLAY_BCKL, OUTPUT);
    digitalWrite(DISPLAY_BCKL, HIGH);

    const int misoPin = (ST7789_SPI_BUS_MISO < 0) ? -1 : ST7789_SPI_BUS_MISO;
    tftSpi.begin(ST7789_SPI_BUS_SCLK, misoPin, ST7789_SPI_BUS_MOSI, ST7789_SPI_CONFIG_CS);
    tft.init(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    tft.setSPISpeed(ST7789_SPI_CONFIG_PCLK_HZ);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);

    gSink = std::make_unique<St7789TextSink>(tft, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    gEePub.setDisplay(gSink.get());
    return true;
}

bool mountSdcard() {
    spi_bus_config_t buscfg = {
        .mosi_io_num = static_cast<gpio_num_t>(TF_SPI_MOSI),
        .miso_io_num = static_cast<gpio_num_t>(TF_SPI_MISO),
        .sclk_io_num = static_cast<gpio_num_t>(TF_SPI_SCLK),
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 4096,
    };

    spi_bus_initialize(static_cast<spi_host_device_t>(TF_SPI_HOST), &buscfg, SPI_DMA_CH_AUTO);

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = TF_SPI_HOST;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = static_cast<gpio_num_t>(TF_CS);
    slot_config.host_id = static_cast<spi_host_device_t>(TF_SPI_HOST);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t* card = nullptr;
    esp_err_t status = esp_vfs_fat_sdspi_mount(kMountPoint, &host, &slot_config, &mount_config, &card);
    gSdMounted = (status == ESP_OK);
    if (!gSdMounted) {
        Serial.printf("[EePub][ERR] Failed to mount SD: %s\n", esp_err_to_name(status));
    }
    return gSdMounted;
}

bool fileExists(const std::string& path) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        return false;
    }
    fclose(f);
    return true;
}

bool findFirstEpub(std::string& outPath) {
    static const char* kPreferred[] = {
        "/sdcard/Accessible EPUB 3 _ Matt Garrish.epub",
        "/sdcard/book.epub",
        "/sdcard/examples/Accessible EPUB 3 _ Matt Garrish.epub"
    };

    for (const char* candidate : kPreferred) {
        if (fileExists(candidate)) {
            outPath = candidate;
            return true;
        }
    }

    DIR* dir = opendir(kMountPoint);
    if (!dir) {
        return false;
    }

    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG && entry->d_type != DT_UNKNOWN) {
            continue;
        }
        std::string name(entry->d_name);
        auto dot = name.find_last_of('.');
        if (dot == std::string::npos) {
            continue;
        }
        std::string ext = name.substr(dot + 1);
        for (auto& ch : ext) {
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        }
        if (ext == "epub") {
            outPath = std::string(kMountPoint) + "/" + name;
            closedir(dir);
            return true;
        }
    }

    closedir(dir);
    return false;
}

void renderCurrentChapter() {
    if (!gRenderPending) {
        return;
    }
    gEePub.render();
    gRenderPending = false;
}

void handleNavigationButton() {
    static bool previousState = false;
    static std::uint32_t lastChangeMs = 0;
    const bool pressed = digitalRead(kNavButton) == LOW;
    const std::uint32_t now = millis();
    if (pressed && !previousState && (now - lastChangeMs) > 250) {
        if (!gEePub.nextChapter()) {
            gEePub.gotoChapter(0);
        }
        gRenderPending = true;
        lastChangeMs = now;
    }
    previousState = pressed;
}

void setup() {
    Serial.begin(115200);
    static const EePubHal kHal = { nullptr, halLog, halHeapFree, halMillisHook };
    EePubHalInstall(&kHal);

    gEePub.begin(false);
    gEePub.setPageWidth(DISPLAY_WIDTH / 7);
    gEePub.setMemorySoftLimit(400000);

    initDisplay();

    pinMode(kNavButton, INPUT_PULLUP);

    if (mountSdcard() && findFirstEpub(gCurrentBook)) {
        Serial.printf("[EePub][INF] Loading %s\n", gCurrentBook.c_str());
        if (gEePub.loadFile(gCurrentBook.c_str())) {
            gRenderPending = true;
        } else {
            Serial.println("[EePub][ERR] Failed to load EPUB file.");
        }
    } else {
        Serial.println("[EePub][WRN] Place an EPUB file on the SD card to render.");
    }
}

void loop() {
    handleNavigationButton();
    renderCurrentChapter();
    delay(16);
}
