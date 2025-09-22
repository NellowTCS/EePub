#include "LexborWrapper.h"
#include <iostream>
#include <functional>
#include <lexbor/dom/dom.h>
#include <lexbor/html/html.h>

bool LexborWrapper::parse(const std::string &html) {
    // Create parser
    lxb_html_parser_t *parser = lxb_html_parser_create();
    if (!parser) {
        std::cerr << "[LexborWrapper] Failed to create parser" << std::endl;
        return false;
    }

    // Parse HTML
    lxb_html_document_t *doc = lxb_html_parse(parser,
        reinterpret_cast<const lxb_char_t*>(html.c_str()),
        html.size());
    if (!doc) {
        std::cerr << "[LexborWrapper] Failed to parse HTML" << std::endl;
        lxb_html_parser_destroy(parser);
        return false;
    }

    lxb_dom_document_t *doc_dom = &doc->dom_document;
    lxb_dom_node_t *root = lxb_dom_document_root(doc_dom);
    if (!root) {
        std::cerr << "[LexborWrapper] Failed to get root node" << std::endl;
        lxb_html_document_destroy(doc);
        lxb_html_parser_destroy(parser);
        return false;
    }

    // Debugging logs
    std::cout << "[LexborWrapper] HTML size: " << html.size() << std::endl;
    std::cout << "[LexborWrapper] HTML content: " << html << std::endl;

    // Recursive traversal
    std::function<void(lxb_dom_node_t*)> traverse;
    traverse = [&](lxb_dom_node_t* node) {
        if (!node) return;

        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            lxb_dom_element_t *el = lxb_dom_interface_element(node);
            size_t tag_len;
            const lxb_char_t *tag = lxb_dom_element_local_name(el, &tag_len);
            std::string tagName((const char*)tag, tag_len);

            if (tagName == "p") {
                lxb_dom_node_t *textNode = lxb_dom_node_first_child(node);
                if (textNode && textNode->type == LXB_DOM_NODE_TYPE_TEXT) {
                    size_t len = 0;
                    const lxb_char_t *data = lxb_dom_node_text_content(textNode, &len);
                    if (data) {
                        std::cout << "[Lexbor] " << std::string((const char*)data, len) << std::endl;
                    }
                }
            }
        }

        for (lxb_dom_node_t *child = node->first_child; child; child = child->next) {
            traverse(child);
        }
    };

    traverse(root);

    lxb_html_document_destroy(doc);
    lxb_html_parser_destroy(parser);

    return true;
}
