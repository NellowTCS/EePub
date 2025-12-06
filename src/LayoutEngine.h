#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "LexborWrapper.h"

class LayoutEngine {
public:
    struct LayoutConfig {
        std::size_t maxLineChars = 80;
        std::size_t headingMaxLineChars = 60;
        std::size_t indentSpaces = 2;
    };

    struct LayoutLine {
        std::string text;
        bool heading = false;
        bool listItem = false;
    };

    struct LayoutPage {
        std::vector<LayoutLine> lines;
    };

    LayoutEngine() = default;

    LayoutPage paginate(const std::string& html, const LayoutConfig& config) const;

private:
    static std::string makeIndent(std::size_t spaces);
    static std::vector<LayoutLine> wrapBlock(const LexborWrapper::TextBlock& block,
                                             const LayoutConfig& config);

    mutable LexborWrapper parser_;
};
