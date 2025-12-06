#include "LexborWrapper.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <vector>
#include <lexbor/core/base.h>
#include <lexbor/core/lexbor.h>
#include <lexbor/dom/dom.h>
#include <lexbor/html/html.h>

namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string collapseWhitespace(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    bool inSpace = false;
    for (char ch : input) {
        if (std::isspace(static_cast<unsigned char>(ch))) {
            if (!inSpace && !result.empty()) {
                result.push_back(' ');
            }
            inSpace = true;
        } else {
            result.push_back(ch);
            inSpace = false;
        }
    }
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

int headingLevelFromTag(const std::string& tag) {
    if (tag.size() == 2 && tag[0] == 'h' && std::isdigit(static_cast<unsigned char>(tag[1]))) {
        return tag[1] - '0';
    }
    return 0;
}

bool isBlockTag(const std::string& tag) {
    static const std::vector<std::string> kBlockTags = {
        "p", "li", "blockquote", "section", "article"
    };
    return std::find(kBlockTags.begin(), kBlockTags.end(), tag) != kBlockTags.end();
}

} // namespace

bool LexborWrapper::extractBlocks(const std::string& html, std::vector<TextBlock>& outBlocks) const {
    outBlocks.clear();

    lxb_html_parser_t* parser = lxb_html_parser_create();
    if (!parser) {
        std::cerr << "[LexborWrapper] Failed to create parser" << std::endl;
        return false;
    }

    if (lxb_html_parser_init(parser) != LXB_STATUS_OK) {
        std::cerr << "[LexborWrapper] Failed to init parser" << std::endl;
        lxb_html_parser_destroy(parser);
        return false;
    }

    lxb_html_document_t* doc = lxb_html_parse(parser,
        reinterpret_cast<const lxb_char_t*>(html.c_str()),
        html.size());
    if (!doc) {
        std::cerr << "[LexborWrapper] Failed to parse HTML" << std::endl;
        lxb_html_parser_destroy(parser);
        return false;
    }

    lxb_dom_document_t* docDom = &doc->dom_document;
    lxb_dom_node_t* root = lxb_dom_document_root(docDom);
    if (!root) {
        std::cerr << "[LexborWrapper] Failed to get root node" << std::endl;
        lxb_html_document_destroy(doc);
        lxb_html_parser_destroy(parser);
        return false;
    }

    auto captureBlock = [&](lxb_dom_node_t* node, const std::string& tag) {
        size_t len = 0;
        const lxb_char_t* data = lxb_dom_node_text_content(node, &len);
        if (!data || len == 0) {
            return;
        }
        std::string raw(reinterpret_cast<const char*>(data), len);

        std::string collapsed = collapseWhitespace(raw);
        if (collapsed.empty()) {
            return;
        }

        TextBlock block;
        block.text = std::move(collapsed);
        block.headingLevel = headingLevelFromTag(tag);
        block.heading = block.headingLevel > 0;
        block.listItem = (tag == "li");
        outBlocks.push_back(std::move(block));
    };

    std::function<void(lxb_dom_node_t*)> traverse = [&](lxb_dom_node_t* node) {
        if (!node) {
            return;
        }

        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            lxb_dom_element_t* el = lxb_dom_interface_element(node);
            size_t tagLen = 0;
            const lxb_char_t* tag = lxb_dom_element_local_name(el, &tagLen);
            std::string tagName(reinterpret_cast<const char*>(tag), tagLen);
            tagName = toLower(tagName);

            if (isBlockTag(tagName) || headingLevelFromTag(tagName) > 0) {
                captureBlock(node, tagName);
            }
        }

        for (lxb_dom_node_t* child = node->first_child; child; child = child->next) {
            traverse(child);
        }
    };

    traverse(root);

    lxb_html_document_destroy(doc);
    lxb_html_parser_destroy(parser);

    return !outBlocks.empty();
}
