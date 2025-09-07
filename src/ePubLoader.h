#pragma once
#include <string>
#include <vector>

class EpubLoader {
public:
    EpubLoader();
    ~EpubLoader();

    bool load(const std::string& path);     // Load EPUB file
    std::string getChapter(int index);      // Get chapter HTML
    int chapterCount() const;               // Total chapters

private:
    std::vector<std::string> chapters;
};
