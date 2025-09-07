#pragma once
#include <string>

class EePub {
public:
    EePub();
    ~EePub();

    bool begin(bool useJS = false);        // Initialize library
    bool loadFile(const char* path);       // Load EPUB from filesystem
    bool nextChapter();                    // Next chapter
    bool prevChapter();                    // Previous chapter
    void render();                         // Render current chapter
    void runJS(const char* script);        // Optional JS execution
    void setDisplay(void* display);        // Set display context (platform-specific)

    private:
    class Impl;
    Impl* pImpl;
};
