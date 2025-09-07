#include "Renderer.h"
#include <iostream>

void Renderer::renderPage(const std::string& html) {
    std::cout << "[Renderer] Rendering HTML..." << std::endl;

    if (!parser.parse(html)) {
        std::cerr << "[Renderer] Failed to parse HTML" << std::endl;
        return;
    }

    // Example: here you could render to the actual display if display != nullptr
    if (display) {
        std::cout << "[Renderer] Would render to display (not implemented)" << std::endl;
    } else {
        std::cout << "[Renderer] No display provided; skipping rendering output" << std::endl;
    }
}
