#pragma once
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "DisplaySink.h"
#include "ePubLoader.h"
#include "MemoryMonitor.h"

class EePub {
public:
    EePub();
    ~EePub();

    bool begin(bool useJS = false);        // Initialize library
    bool loadFile(const char* path);       // Load EPUB from filesystem
    bool nextChapter();                    // Next chapter
    bool prevChapter();                    // Previous chapter
    bool gotoChapter(int index);           // Jump to specific chapter
    void render();                         // Render current chapter
    void runJS(const char* script);        // Optional JS execution
    void setDisplay(DisplaySink* sink);    // Set display context (platform-specific)
    void setPageWidth(std::size_t columns);

    int chapterCount() const;
    int currentChapterIndex() const;
    std::string title() const;
    const EpubLoader::ChapterData* currentChapter() const;
    const std::vector<EpubLoader::ChapterData>& chapters() const;

    EePubMemoryStats memoryStats() const;
    void setMemorySoftLimit(std::size_t bytes);

    private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
