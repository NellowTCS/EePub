#include "Renderer.h"

#include <algorithm>
#include <cctype>
#include <iostream>

void Renderer::renderPage(const std::string& html) {
    LayoutEngine::LayoutConfig config;
    config.maxLineChars = pageWidth;
    config.headingMaxLineChars = pageWidth > 40 ? pageWidth - 20 : pageWidth;

    auto page = layoutEngine.paginate(html, config);
    if (page.lines.empty()) {
        std::cerr << "[Renderer] Nothing to render" << std::endl;
        return;
    }

    if (displaySink) {
        displaySink->beginFrame(page.lines.size());
        for (std::size_t i = 0; i < page.lines.size(); ++i) {
            displaySink->drawLine(i, page.lines[i]);
        }
        displaySink->endFrame();
        return;
    }

    std::cout << "[Renderer] CLI fallback (" << page.lines.size() << " lines)" << std::endl;
    for (const auto& line : page.lines) {
        if (line.text.empty()) {
            std::cout << std::endl;
            continue;
        }

        if (line.heading) {
            std::string heading = line.text;
            std::transform(heading.begin(), heading.end(), heading.begin(), [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            });
            std::cout << heading << std::endl;
        } else {
            std::cout << line.text << std::endl;
        }
    }
}
