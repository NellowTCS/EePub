#include "EePubDesktop.h"
#include "hal/Hal.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace {

struct CliOptions {
    bool headless = false;
    bool renderAll = false;
    int startChapter = 0;
    std::size_t softLimitBytes = 0;
};

void desktopLog(EePubLogLevel level, const char* message, void*) {
    const char* tag = (level == EePubLogLevel::Error)
        ? "ERR"
        : (level == EePubLogLevel::Warn ? "WRN" : "INF");
    if (level == EePubLogLevel::Error) {
        std::cerr << "[EePubDesktop][" << tag << "] " << message << std::endl;
    } else {
        std::cout << "[EePubDesktop][" << tag << "] " << message << std::endl;
    }
}

std::size_t desktopHeapFree(void*) {
    return 0;
}

std::uint64_t desktopUptime(void*) {
    static const auto start = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

const EePubHal kDesktopHal = {
    nullptr,
    desktopLog,
    desktopHeapFree,
    desktopUptime,
};

void printUsage() {
    std::cout << "Usage: EePubDesktop <file.epub> [--headless] [--render-all] [--chapter N] [--mem-soft-limit BYTES]\n";
}

bool parseArgs(int argc, char** argv, std::string& epubPath, CliOptions& options) {
    if (argc < 2) {
        printUsage();
        return false;
    }

    epubPath = argv[1];
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--headless") {
            options.headless = true;
        } else if (arg == "--render-all") {
            options.renderAll = true;
        } else if (arg == "--chapter" && i + 1 < argc) {
            try {
                options.startChapter = std::stoi(argv[++i]);
            } catch (const std::exception& ex) {
                std::cerr << "Invalid chapter value: " << ex.what() << std::endl;
                return false;
            }
        } else if (arg == "--mem-soft-limit" && i + 1 < argc) {
            try {
                options.softLimitBytes = static_cast<std::size_t>(std::stoll(argv[++i]));
            } catch (const std::exception& ex) {
                std::cerr << "Invalid mem-soft-limit value: " << ex.what() << std::endl;
                return false;
            }
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage();
            return false;
        }
    }

    return true;
}

void printChapterList(const EePub& epub) {
    const auto& chapters = epub.chapters();
    std::cout << "\nChapters (" << chapters.size() << "):" << std::endl;
    for (std::size_t i = 0; i < chapters.size(); ++i) {
        std::cout << "  [" << i << "] " << chapters[i].title << std::endl;
    }
}

void printStats(const EePub& epub) {
    auto stats = epub.memoryStats();
    std::cout << "Current memory: " << stats.currentBytes << " bytes" << std::endl;
    std::cout << "Peak memory: " << stats.peakBytes << " bytes" << std::endl;
}

std::size_t detectTerminalWidth() {
    if (const char* envCols = std::getenv("COLUMNS")) {
        int value = std::atoi(envCols);
        if (value > 20) {
            return static_cast<std::size_t>(value);
        }
    }
#if defined(TIOCGWINSZ)
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return static_cast<std::size_t>(ws.ws_col);
    }
#endif
    return 80;
}

void runInteractive(EePub& epub) {
    std::cout << "Interactive mode. Commands: list, next, prev, goto <n>, render, stats, quit" << std::endl;
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break;
        }
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        if (command.empty()) {
            continue;
        }
        std::transform(command.begin(), command.end(), command.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        if (command == "quit" || command == "q") {
            break;
        } else if (command == "list" || command == "l") {
            printChapterList(epub);
        } else if (command == "next" || command == "n") {
            if (!epub.nextChapter()) {
                std::cout << "Already at final chapter." << std::endl;
            } else {
                std::cout << "Chapter " << epub.currentChapterIndex() << ": "
                          << (epub.currentChapter() ? epub.currentChapter()->title : "?") << std::endl;
            }
        } else if (command == "prev" || command == "p") {
            if (!epub.prevChapter()) {
                std::cout << "Already at first chapter." << std::endl;
            } else {
                std::cout << "Chapter " << epub.currentChapterIndex() << ": "
                          << (epub.currentChapter() ? epub.currentChapter()->title : "?") << std::endl;
            }
        } else if (command == "goto" || command == "g") {
            int target;
            if (!(iss >> target)) {
                std::cout << "Usage: goto <chapterIndex>" << std::endl;
                continue;
            }
            if (!epub.gotoChapter(target)) {
                std::cout << "Invalid chapter index." << std::endl;
            } else {
                std::cout << "Chapter " << target << ": "
                          << (epub.currentChapter() ? epub.currentChapter()->title : "?") << std::endl;
            }
        } else if (command == "render" || command == "r") {
            epub.render();
        } else if (command == "stats") {
            printStats(epub);
        } else {
            std::cout << "Unknown command: " << command << std::endl;
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    EePubHalInstall(&kDesktopHal);

    std::string epubPath;
    CliOptions options;
    if (!parseArgs(argc, argv, epubPath, options)) {
        return 1;
    }

    EePubDesktop epub;
    epub.begin(false);
    epub.setPageWidth(detectTerminalWidth());
    if (options.softLimitBytes > 0) {
        epub.setMemorySoftLimit(options.softLimitBytes);
    }

    if (!epub.loadFile(epubPath.c_str())) {
        std::cerr << "Failed to load EPUB: " << epubPath << std::endl;
        return 2;
    }

    if (options.startChapter > 0) {
        epub.gotoChapter(options.startChapter);
    }

    std::cout << "Loaded: " << epub.title() << std::endl;
    std::cout << "Chapters: " << epub.chapterCount() << std::endl;

    if (options.headless) {
        if (options.renderAll) {
            for (int i = 0; i < epub.chapterCount(); ++i) {
                epub.gotoChapter(i);
                epub.render();
            }
        } else {
            epub.render();
        }
        printStats(epub);
        return 0;
    }

    printChapterList(epub);
    runInteractive(epub);
    printStats(epub);

    return 0;
}
