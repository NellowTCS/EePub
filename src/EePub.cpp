#include "EePub.h"
#include "Renderer.h"
#include <iostream>

class EePub::Impl {
public:
    Renderer renderer;
    int currentChapter = 0;

    bool load(const char* path) {
        std::cout << "[EePub] Loading file: " << path << std::endl;
        return true;
    }

    void render() {
        std::cout << "[EePub] Rendering chapter " << currentChapter << std::endl;
        renderer.renderPage("Dummy HTML from EPUB");
    }
};

EePub::EePub() : pImpl(new Impl) {}
EePub::~EePub() { delete pImpl; }

bool EePub::begin(bool useJS) {
    std::cout << "[EePub] begin (JS=" << (useJS ? "enabled" : "disabled") << ")" << std::endl;
    return true;
}

bool EePub::loadFile(const char* path) {
    return pImpl->load(path);
}

bool EePub::nextChapter() {
    pImpl->currentChapter++;
    std::cout << "[EePub] Next chapter: " << pImpl->currentChapter << std::endl;
    return true;
}

bool EePub::prevChapter() {
    if (pImpl->currentChapter > 0) {
        pImpl->currentChapter--;
        std::cout << "[EePub] Previous chapter: " << pImpl->currentChapter << std::endl;
        return true;
    }
    return false;
}

void EePub::render() {
    pImpl->render();
}

void EePub::runJS(const char* script) {
    std::cout << "[EePub] Running JS: " << script << std::endl;
}

void EePub::setDisplay(void* display) {
    pImpl->renderer.setDisplay(display);
}
