#include "EePubDesktop.h"
#include <iostream>

int main(int argc, char** argv) {
    EePubDesktop epub;

    epub.begin(false);

    if (argc > 1) {
        if (epub.loadFile(argv[1])) {
            std::cout << "Loaded EPUB: " << argv[1] << std::endl;
            epub.render();
            epub.nextChapter();
            epub.render();
        } else {
            std::cerr << "Failed to load EPUB." << std::endl;
        }
    } else {
        std::cout << "Usage: EePubDesktop <file.epub>" << std::endl;
    }

    return 0;
}
