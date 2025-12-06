#include "EePub.h"
#include "LayoutEngine.h"

#include <sstream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/bind.h>
#endif

class EePubWasmBinding {
public:
    EePubWasmBinding() {
        engine.begin(false);
    }

    bool begin(bool useJS) {
        return engine.begin(useJS);
    }

    bool load(const std::string& path) {
        return engine.loadFile(path.c_str());
    }

    std::string getTitle() const {
        return engine.title();
    }

    std::vector<std::string> chapterTitles() const {
        std::vector<std::string> titles;
        for (const auto& chapter : engine.chapters()) {
            titles.push_back(chapter.title);
        }
        return titles;
    }

    bool next() { return engine.nextChapter(); }
    bool prev() { return engine.prevChapter(); }
    bool go(int index) { return engine.gotoChapter(index); }

    int chapterCount() const { return engine.chapterCount(); }
    int currentChapterIndex() const { return engine.currentChapterIndex(); }

    std::string renderCurrent(std::size_t columns = 80) {
        const auto* chapter = engine.currentChapter();
        if (!chapter) {
            return {};
        }

        LayoutEngine::LayoutConfig config;
        config.maxLineChars = columns;
        config.headingMaxLineChars = columns > 40 ? columns - 20 : columns;

        auto page = layout.paginate(chapter->content, config);
        std::ostringstream oss;
        for (const auto& line : page.lines) {
            oss << line.text << '\n';
        }
        return oss.str();
    }

    EePubMemoryStats getMemoryStats() const {
        return engine.memoryStats();
    }

private:
    EePub engine;
    LayoutEngine layout;
};

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_BINDINGS(eepub_module) {
    emscripten::register_vector<std::string>("StringVector");

    emscripten::value_object<EePubMemoryStats>("EePubMemoryStats")
        .field("currentBytes", &EePubMemoryStats::currentBytes)
        .field("peakBytes", &EePubMemoryStats::peakBytes);

    emscripten::class_<EePubWasmBinding>("EePubEngine")
        .constructor<>()
        .function("begin", &EePubWasmBinding::begin)
        .function("load", &EePubWasmBinding::load)
        .function("getTitle", &EePubWasmBinding::getTitle)
        .function("chapterTitles", &EePubWasmBinding::chapterTitles)
        .function("next", &EePubWasmBinding::next)
        .function("prev", &EePubWasmBinding::prev)
        .function("go", &EePubWasmBinding::go)
        .function("chapterCount", &EePubWasmBinding::chapterCount)
        .function("currentChapterIndex", &EePubWasmBinding::currentChapterIndex)
        .function("renderCurrent", &EePubWasmBinding::renderCurrent)
        .function("memoryStats", &EePubWasmBinding::getMemoryStats);
}
#endif
