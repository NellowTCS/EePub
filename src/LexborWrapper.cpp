#include "LexborWrapper.h"
#include <lexbor/html/html.h>
#include <iostream>
#include <functional>

bool LexborWrapper::parse(const std::string& html) {
    // Create HTML parser
    lxb_html_parser_t *parser = lxb_html_parser_create();
    if (!parser) {
        std::cerr << "[Lexbor] Failed to create parser" << std::endl;
        return false;
    }

    // Create HTML document
    lxb_html_document_t *doc = lxb_html_document_create(parser);
    if (!doc) {
        std::cerr << "[Lexbor] Failed to create document" << std::endl;
        lxb_html_parser_destroy(parser);
        return false;
    }

    // Parse HTML string
    lxb_status_t status = lxb_html_parse(parser,
                                        (const lxb_char_t*)html.c_str(),
                                        html.size());
    if (status != LXB_STATUS_OK) {
        std::cerr << "[Lexbor] Parsing failed with status: " << status << std::endl;
        lxb_html_document_destroy(doc);
        lxb_html_parser_destroy(parser);
        return false;
    }

    // Get the DOM document from parser
    doc = lxb_html_parser_document(parser);
    if (!doc) {
        std::cerr << "[Lexbor] Failed to get DOM document" << std::endl;
        lxb_html_parser_destroy(parser);
        return false;
    }

    // Get root node
    lxb_dom_node_t *root = lxb_dom_document_root((lxb_dom_document_t*)doc);
    if (!root) {
        std::cerr << "[Lexbor] Document root is null" << std::endl;
        lxb_html_document_destroy(doc);
        lxb_html_parser_destroy(parser);
        return false;
    }

    // Traverse DOM safely
    std::function<void(lxb_dom_node_t*)> traverse;
    traverse = [&](lxb_dom_node_t* node) {
        if (!node) return;

        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            lxb_dom_element_t *el = (lxb_dom_element_t*)node;
            size_t len = 0;
            const lxb_char_t *tag = lxb_dom_element_local_name(el, &len);
            if (tag && std::string((const char*)tag, len) == "p") {
                lxb_dom_node_t *child = lxb_dom_node_first_child(node);
                while (child) {
                    if (child->type == LXB_DOM_NODE_TYPE_TEXT) {
                        lxb_dom_text_t *textNode = (lxb_dom_text_t*)child;
                        size_t text_len = 0;
                        const lxb_char_t *data = lxb_dom_text_data(textNode, &text_len);
                        if (data) {
                            std::cout << "[Lexbor] " << std::string((const char*)data, text_len) << std::endl;
                        }
                    }
                    child = lxb_dom_node_next_sibling(child);
                }
            }
        }

        // Recurse into children
        lxb_dom_node_t *child = lxb_dom_node_first_child(node);
        while (child) {
            traverse(child);
            child = lxb_dom_node_next_sibling(child);
        }
    };

    traverse(root);

    // Clean up
    lxb_html_document_destroy(doc);
    lxb_html_parser_destroy(parser);
    return true;
}
