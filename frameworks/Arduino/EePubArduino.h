#pragma once

#include "DisplaySink.h"
#include "EePub.h"

#include <TFT_eSPI.h>

class TftTextSink : public DisplaySink {
public:
    explicit TftTextSink(TFT_eSPI& tft) : panel(tft) {}

    void beginFrame(std::size_t) override {
        cursorY = topMargin;
        panel.fillScreen(TFT_BLACK);
        panel.setTextColor(TFT_WHITE, TFT_BLACK);
        panel.setTextSize(1);
        panel.setTextWrap(false);
    }

    void drawLine(std::size_t, const LayoutEngine::LayoutLine& line) override {
        panel.setCursor(leftMargin, cursorY);
        uint16_t color = line.heading ? headingColor : textColor;
        panel.setTextColor(color, TFT_BLACK);
        panel.print(line.text.c_str());
        cursorY += lineHeight;
        if (cursorY >= static_cast<int>(panel.height())) {
            cursorY = topMargin;
        }
    }

    void endFrame() override {}

private:
    TFT_eSPI& panel;
    int cursorY = 0;
    static constexpr int topMargin = 8;
    static constexpr int leftMargin = 4;
    static constexpr int lineHeight = 14;
    static constexpr uint16_t headingColor = TFT_CYAN;
    static constexpr uint16_t textColor = TFT_WHITE;
};

class EePubArduino : public EePub {
public:
    explicit EePubArduino(TFT_eSPI& display)
        : sink(display) {
        setDisplay(&sink);
    }

private:
    TftTextSink sink;
};
