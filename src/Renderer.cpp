#include "Renderer.h"
#include <iostream>

void Renderer::renderPage(const std::string& html) {
    std::cout << "[Renderer] Rendering HTML..." << std::endl;

    if (!parser.parse(html)) {
        std::cerr << "[Renderer] Failed to parse HTML" << std::endl;
        return;
    }

    if (display) {
        // Simple rendering logic: output HTML content to the console
        std::cout << "[Renderer] Rendering to display: " << html << std::endl;
    } else {
        std::cout << "[Renderer] No display provided; skipping rendering output" << std::endl;
    }
}
