#pragma once
#include <string>
#include <vector>

class EpubLoader {
public:
    struct PackageMetadata {
        std::string title;
        std::string language;
        std::string identifier;
        std::string creator;
    };

    struct ChapterData {
        std::string id;
        std::string title;
        std::string href;
        std::string mediaType;
        std::string content;
    };

    EpubLoader();
    ~EpubLoader();

    bool load(const std::string& path);
    const PackageMetadata& metadata() const;
    const ChapterData* getChapter(int index) const;
    int chapterCount() const;
    const std::vector<ChapterData>& chapters() const;

private:
    std::vector<ChapterData> chapters_;
    PackageMetadata metadata_;
};
