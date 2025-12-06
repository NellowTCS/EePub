#pragma once

#include <cstddef>
#include <string>

#include "DisplaySink.h"
#include "LayoutEngine.h"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    void setDisplay(DisplaySink* sink) {
        displaySink = sink;
    }
    bool isDisplaySet() const { return displaySink != nullptr; }

    void setPageWidth(std::size_t columns) { pageWidth = columns; }

    void renderPage(const std::string& html);

private:
    DisplaySink* displaySink = nullptr;
    std::size_t pageWidth = 80;
    LayoutEngine layoutEngine;
};
