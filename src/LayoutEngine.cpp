#include "LayoutEngine.h"

#include <sstream>

namespace {

std::vector<std::string> splitWords(const std::string& text) {
    std::istringstream iss(text);
    std::vector<std::string> words;
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

} // namespace

LayoutEngine::LayoutPage LayoutEngine::paginate(const std::string& html,
                                                const LayoutConfig& config) const {
    std::vector<LexborWrapper::TextBlock> blocks;
    if (!parser_.extractBlocks(html, blocks)) {
        return {};
    }

    LayoutPage page;
    for (const auto& block : blocks) {
        auto lines = wrapBlock(block, config);
        page.lines.insert(page.lines.end(), lines.begin(), lines.end());
        page.lines.push_back({"", false, false});
    }

    if (!page.lines.empty() && page.lines.back().text.empty()) {
        page.lines.pop_back();
    }

    return page;
}

std::string LayoutEngine::makeIndent(std::size_t spaces) {
    return std::string(spaces, ' ');
}

std::vector<LayoutEngine::LayoutLine> LayoutEngine::wrapBlock(
    const LexborWrapper::TextBlock& block,
    const LayoutConfig& config) {
    const std::size_t lineWidth = block.heading ? config.headingMaxLineChars : config.maxLineChars;
    const std::size_t effectiveWidth = lineWidth < 20 ? 20 : lineWidth;
    const std::string bullet = block.listItem ? "• " : "";
    const std::string paragraphIndent = (!block.heading && !block.listItem)
        ? makeIndent(config.indentSpaces)
        : "";
    const std::string continuationIndent = block.listItem
        ? makeIndent(bullet.size())
        : paragraphIndent;

    auto words = splitWords(block.text);
    std::vector<LayoutLine> lines;
    if (words.empty()) {
        return lines;
    }

    std::string current = block.listItem ? bullet : paragraphIndent;
    bool hasContent = false;
    auto appendLine = [&]() {
        if (hasContent) {
            lines.push_back({current, block.heading, block.listItem});
        }
        current = continuationIndent;
        hasContent = false;
    };

    for (const auto& word : words) {
        const bool needsSpace = !current.empty() && current.back() != ' ';
        std::size_t prospective = current.size() + word.size() + (needsSpace ? 1 : 0);
        if (prospective > effectiveWidth && hasContent) {
            appendLine();
        }
        if (!current.empty() && current.back() != ' ') {
            current.push_back(' ');
        }
        current += word;
        hasContent = true;
    }

    appendLine();
    return lines;
}
