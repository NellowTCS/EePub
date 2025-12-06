#pragma once

#include <cstddef>

#include "LayoutEngine.h"

class DisplaySink {
public:
    virtual ~DisplaySink() = default;

    virtual void beginFrame(std::size_t lineCount) = 0;
    virtual void drawLine(std::size_t index, const LayoutEngine::LayoutLine& line) = 0;
    virtual void endFrame() = 0;
};
