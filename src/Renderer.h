#pragma once
#include <string>
#include "LexborWrapper.h"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    void setDisplay(void* display) {
        this->display = display;
    }

    void renderPage(const std::string& html);

private:
    void* display = nullptr;      // Platform-specific display pointer
    LexborWrapper parser;         // HTML parser
};
