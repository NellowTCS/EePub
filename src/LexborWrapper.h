#pragma once

#include <string>
#include <vector>

class LexborWrapper {
public:
    struct TextBlock {
        std::string text;
        bool heading = false;
        int headingLevel = 0;
        bool listItem = false;
    };

    bool extractBlocks(const std::string& html, std::vector<TextBlock>& outBlocks) const;
};
