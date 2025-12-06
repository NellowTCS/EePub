#include "EePub.h"
#include "Renderer.h"
#include "hal/Hal.h"

#include <iostream>
#include <optional>
#include <string>

namespace {

void emitLog(EePubLogLevel level, const std::string& message) {
    const EePubHal* hal = EePubHalCurrent();
    if (hal && hal->log) {
        hal->log(level, message.c_str(), hal->userContext);
        return;
    }

    if (level == EePubLogLevel::Error) {
        std::cerr << message << std::endl;
    } else {
        std::cout << message << std::endl;
    }
}

void logInfo(const std::string& message) {
    emitLog(EePubLogLevel::Info, message);
}

void logWarn(const std::string& message) {
    emitLog(EePubLogLevel::Warn, message);
}

void logError(const std::string& message) {
    emitLog(EePubLogLevel::Error, message);
}

}

class EePub::Impl {
public:
    Renderer renderer;
    EpubLoader loader;
    int currentChapter = 0;
    bool jsEnabled = false;
    MemoryMonitor& memoryMonitor = MemoryMonitor::instance();
    std::size_t configuredSoftLimit = 0;

    bool load(const std::string& path) {
        logInfo("[EePub] Loading file: " + path);
        bool ok = loader.load(path);
        if (ok) {
            currentChapter = 0;
            updateMonitor();
        }
        return ok;
    }

    void render() {
        const auto* chapter = loader.getChapter(currentChapter);
        if (!chapter) {
            logError("[EePub] No chapter loaded for index " + std::to_string(currentChapter));
            return;
        }

        logInfo("[EePub] Rendering chapter " + std::to_string(currentChapter) +
                " (" + chapter->title + ")");

        if (!renderer.isDisplaySet()) {
            logWarn("[EePub] Display sink not set. Falling back to CLI output.");
        }

        renderer.renderPage(chapter->content);
    }

    void updateMonitor() {
        std::size_t totalBytes = 0;
        for (const auto& chapter : loader.chapters()) {
            totalBytes += chapter.content.size();
        }
        memoryMonitor.updateVirtualUsage(totalBytes);
    }
};

EePub::EePub() : pImpl(std::make_unique<Impl>()) {}
EePub::~EePub() = default;

bool EePub::begin(bool useJS) {
    pImpl->jsEnabled = useJS;
    logInfo(std::string("[EePub] begin (JS=") + (useJS ? "enabled" : "disabled") + ")");
    return true;
}

bool EePub::loadFile(const char* path) {
    if (!path) {
        return false;
    }
    return pImpl->load(path);
}

bool EePub::nextChapter() {
    if (pImpl->currentChapter + 1 >= chapterCount()) {
        return false;
    }
    pImpl->currentChapter++;
    logInfo("[EePub] Next chapter: " + std::to_string(pImpl->currentChapter));
    return true;
}

bool EePub::prevChapter() {
    if (pImpl->currentChapter == 0) {
        return false;
    }
    pImpl->currentChapter--;
    logInfo("[EePub] Previous chapter: " + std::to_string(pImpl->currentChapter));
    return true;
}

bool EePub::gotoChapter(int index) {
    if (index < 0 || index >= chapterCount()) {
        return false;
    }
    pImpl->currentChapter = index;
    logInfo("[EePub] Jumped to chapter: " + std::to_string(index));
    return true;
}

void EePub::render() {
    pImpl->render();
}

void EePub::runJS(const char* script) {
    if (!script) {
        return;
    }
    logInfo(std::string("[EePub] Running JS: ") + script);
}

void EePub::setDisplay(DisplaySink* sink) {
    pImpl->renderer.setDisplay(sink);
}

void EePub::setPageWidth(std::size_t columns) {
    pImpl->renderer.setPageWidth(columns);
}

int EePub::chapterCount() const {
    return static_cast<int>(pImpl->loader.chapterCount());
}

int EePub::currentChapterIndex() const {
    return pImpl->currentChapter;
}

std::string EePub::title() const {
    return pImpl->loader.metadata().title;
}

const EpubLoader::ChapterData* EePub::currentChapter() const {
    return pImpl->loader.getChapter(pImpl->currentChapter);
}

const std::vector<EpubLoader::ChapterData>& EePub::chapters() const {
    return pImpl->loader.chapters();
}

EePubMemoryStats EePub::memoryStats() const {
    EePubMemoryStats stats;
    auto snapshot = pImpl->memoryMonitor.snapshot();
    stats.currentBytes = snapshot.currentBytes;
    stats.peakBytes = snapshot.peakBytes;
    return stats;
}

void EePub::setMemorySoftLimit(std::size_t bytes) {
    pImpl->configuredSoftLimit = bytes;
    if (bytes == 0) {
        pImpl->memoryMonitor.setSoftLimit(0, nullptr);
        return;
    }
    pImpl->memoryMonitor.setSoftLimit(bytes, [](std::size_t current) {
        logWarn("[MemoryMonitor] Soft limit exceeded: " + std::to_string(current) + " bytes");
    });
}
