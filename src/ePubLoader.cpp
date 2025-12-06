#include "ePubLoader.h"

#include "miniz.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <optional>
#include <unordered_map>
#include <vector>

namespace {

struct ZipGuard {
    mz_zip_archive* archive;
    explicit ZipGuard(mz_zip_archive* a) : archive(a) {}
    ~ZipGuard() {
        if (archive) {
            mz_zip_reader_end(archive);
        }
    }
};

std::string trim(const std::string& value) {
    auto begin = value.find_first_not_of(" \t\n\r");
    auto end = value.find_last_not_of(" \t\n\r");
    if (begin == std::string::npos || end == std::string::npos) {
        return "";
    }
    return value.substr(begin, end - begin + 1);
}

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::unordered_map<std::string, std::string> parseAttributes(const std::string& tag) {
    std::unordered_map<std::string, std::string> attributes;
    if (tag.empty()) {
        return attributes;
    }

    std::size_t pos = 0;
    if (tag[pos] == '<') {
        pos++;
        while (pos < tag.size() && !std::isspace(static_cast<unsigned char>(tag[pos])) && tag[pos] != '>' && tag[pos] != '/') {
            pos++;
        }
    }

    while (pos < tag.size()) {
        while (pos < tag.size() && std::isspace(static_cast<unsigned char>(tag[pos]))) {
            pos++;
        }
        if (pos >= tag.size() || tag[pos] == '/' || tag[pos] == '>') {
            break;
        }

        auto eqPos = tag.find('=', pos);
        if (eqPos == std::string::npos) {
            break;
        }
        std::string name = trim(tag.substr(pos, eqPos - pos));
        pos = eqPos + 1;

        while (pos < tag.size() && std::isspace(static_cast<unsigned char>(tag[pos]))) {
            pos++;
        }
        if (pos >= tag.size()) {
            break;
        }

        char quote = tag[pos];
        std::size_t valueStart = pos;
        std::size_t valueEnd;
        if (quote == '"' || quote == '\'') {
            valueStart = pos + 1;
            valueEnd = tag.find(quote, valueStart);
            if (valueEnd == std::string::npos) {
                break;
            }
            pos = valueEnd + 1;
        } else {
            while (pos < tag.size() && !std::isspace(static_cast<unsigned char>(tag[pos])) && tag[pos] != '/' && tag[pos] != '>') {
                pos++;
            }
            valueEnd = pos;
        }

        if (!name.empty()) {
            attributes[name] = tag.substr(valueStart, valueEnd - valueStart);
        }
    }

    return attributes;
}

std::optional<std::string> readZipEntry(mz_zip_archive* zip, const std::string& entryName) {
    int fileIndex = mz_zip_reader_locate_file(zip, entryName.c_str(), nullptr, 0);
    if (fileIndex < 0) {
        return std::nullopt;
    }

    size_t size = 0;
    void* data = mz_zip_reader_extract_to_heap(zip, fileIndex, &size, 0);
    if (!data || size == 0) {
        if (data) {
            mz_free(data);
        }
        return std::nullopt;
    }

    std::string buffer(static_cast<const char*>(data), size);
    mz_free(data);
    return buffer;
}

std::optional<std::string> extractRootFilePath(const std::string& containerXml) {
    std::size_t search = 0;
    while (true) {
        std::size_t start = containerXml.find("<rootfile", search);
        if (start == std::string::npos) {
            return std::nullopt;
        }
        std::size_t end = containerXml.find('>', start);
        if (end == std::string::npos) {
            return std::nullopt;
        }
        std::size_t attr = containerXml.find("full-path", start);
        if (attr == std::string::npos || attr > end) {
            search = end + 1;
            continue;
        }
        std::size_t quote = containerXml.find_first_of("\"'", attr);
        if (quote == std::string::npos || quote > end) {
            search = end + 1;
            continue;
        }
        char delim = containerXml[quote];
        std::size_t close = containerXml.find(delim, quote + 1);
        if (close == std::string::npos) {
            return std::nullopt;
        }
        return containerXml.substr(quote + 1, close - quote - 1);
    }
}

std::string getTagContent(const std::string& xml, const std::string& tagName) {
    std::size_t startTag = xml.find("<" + tagName);
    if (startTag == std::string::npos) {
        return "";
    }
    std::size_t start = xml.find('>', startTag);
    if (start == std::string::npos) {
        return "";
    }
    start += 1;
    std::size_t endTag = xml.find("</" + tagName + ">", start);
    if (endTag == std::string::npos) {
        return "";
    }
    return xml.substr(start, endTag - start);
}

std::string normalizePath(const std::string& base, const std::string& relative) {
    if (relative.empty()) {
        return relative;
    }
    if (!base.empty() && relative.front() == '/') {
        return relative;
    }

    std::vector<std::string> stack;
    auto pushSegments = [&stack](const std::string& path) {
        std::size_t start = 0;
        while (start < path.size()) {
            std::size_t slash = path.find('/', start);
            std::size_t len = (slash == std::string::npos) ? std::string::npos : slash - start;
            std::string segment = (len == std::string::npos) ? path.substr(start) : path.substr(start, len);
            if (!segment.empty()) {
                if (segment == "..") {
                    if (!stack.empty()) {
                        stack.pop_back();
                    }
                } else if (segment != ".") {
                    stack.push_back(segment);
                }
            }
            if (slash == std::string::npos) {
                break;
            }
            start = slash + 1;
        }
    };

    pushSegments(base);
    pushSegments(relative);

    std::string result;
    for (std::size_t i = 0; i < stack.size(); ++i) {
        result += stack[i];
        if (i + 1 < stack.size()) {
            result += '/';
        }
    }
    return result;
}

std::vector<std::string> extractSelfClosingTags(const std::string& block, const std::string& tagName) {
    std::vector<std::string> tags;
    std::string pattern = "<" + tagName;
    std::size_t pos = 0;
    while ((pos = block.find(pattern, pos)) != std::string::npos) {
        std::size_t end = block.find('>', pos);
        if (end == std::string::npos) {
            break;
        }
        tags.emplace_back(block.substr(pos, end - pos + 1));
        pos = end + 1;
    }
    return tags;
}

std::string getBlock(const std::string& xml, const std::string& tagName) {
    std::size_t start = xml.find("<" + tagName);
    if (start == std::string::npos) {
        return "";
    }
    std::size_t openEnd = xml.find('>', start);
    if (openEnd == std::string::npos) {
        return "";
    }
    std::size_t close = xml.find("</" + tagName + ">", openEnd);
    if (close == std::string::npos) {
        return "";
    }
    return xml.substr(openEnd + 1, close - openEnd - 1);
}

std::string extractHtmlTitle(const std::string& html) {
    std::size_t pos = html.find("<title");
    if (pos == std::string::npos) {
        return "Untitled Chapter";
    }
    std::size_t start = html.find('>', pos);
    if (start == std::string::npos) {
        return "Untitled Chapter";
    }
    start += 1;
    std::size_t end = html.find("</title>", start);
    if (end == std::string::npos) {
        return "Untitled Chapter";
    }
    return trim(html.substr(start, end - start));
}

bool isHtmlMediaType(const std::string& mediaType) {
    auto lower = toLower(mediaType);
    return lower.find("html") != std::string::npos || lower.find("xhtml") != std::string::npos;
}

} // namespace

EpubLoader::EpubLoader() = default;
EpubLoader::~EpubLoader() = default;

bool EpubLoader::load(const std::string& path) {
    chapters_.clear();
    metadata_ = {};

    mz_zip_archive zip;
    std::memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, path.c_str(), 0)) {
        std::cerr << "[EpubLoader] Failed to open EPUB archive: " << path << std::endl;
        return false;
    }

    ZipGuard guard(&zip);

    auto container = readZipEntry(&zip, "META-INF/container.xml");
    if (!container) {
        std::cerr << "[EpubLoader] Missing container.xml" << std::endl;
        return false;
    }

    auto rootFile = extractRootFilePath(*container);
    if (!rootFile) {
        std::cerr << "[EpubLoader] Unable to locate rootfile from container.xml" << std::endl;
        return false;
    }

    auto package = readZipEntry(&zip, *rootFile);
    if (!package) {
        std::cerr << "[EpubLoader] package.opf missing: " << *rootFile << std::endl;
        return false;
    }

    metadata_.title = trim(getTagContent(*package, "dc:title"));
    if (metadata_.title.empty()) {
        metadata_.title = "Untitled";
    }
    metadata_.language = trim(getTagContent(*package, "dc:language"));
    metadata_.identifier = trim(getTagContent(*package, "dc:identifier"));
    metadata_.creator = trim(getTagContent(*package, "dc:creator"));

    std::string manifestBlock = getBlock(*package, "manifest");
    std::string spineBlock = getBlock(*package, "spine");
    if (manifestBlock.empty() || spineBlock.empty()) {
        std::cerr << "[EpubLoader] Invalid OPF manifest or spine" << std::endl;
        return false;
    }

    struct ManifestItem {
        std::string id;
        std::string href;
        std::string mediaType;
    };

    std::unordered_map<std::string, ManifestItem> manifest;

    auto manifestTags = extractSelfClosingTags(manifestBlock, "item");
    for (const auto& tag : manifestTags) {
        auto attributes = parseAttributes(tag);
        auto id = attributes.find("id");
        auto href = attributes.find("href");
        if (id == attributes.end() || href == attributes.end()) {
            continue;
        }
        ManifestItem item;
        item.id = id->second;
        item.href = href->second;
        auto media = attributes.find("media-type");
        if (media != attributes.end()) {
            item.mediaType = media->second;
        }
        manifest[item.id] = item;
    }

    std::vector<std::string> spineOrder;
    auto spineTags = extractSelfClosingTags(spineBlock, "itemref");
    for (const auto& tag : spineTags) {
        auto attributes = parseAttributes(tag);
        auto idref = attributes.find("idref");
        if (idref != attributes.end()) {
            spineOrder.push_back(idref->second);
        }
    }

    if (spineOrder.empty()) {
        std::cerr << "[EpubLoader] Spine is empty" << std::endl;
        return false;
    }

    std::string baseDir;
    auto slash = rootFile->find_last_of('/') ;
    if (slash != std::string::npos) {
        baseDir = rootFile->substr(0, slash + 1);
    }

    for (const auto& idref : spineOrder) {
        auto it = manifest.find(idref);
        if (it == manifest.end()) {
            continue;
        }
        if (!isHtmlMediaType(it->second.mediaType)) {
            continue;
        }
        auto assetPath = normalizePath(baseDir, it->second.href);
        auto html = readZipEntry(&zip, assetPath);
        if (!html) {
            std::cerr << "[EpubLoader] Unable to read chapter asset: " << assetPath << std::endl;
            continue;
        }

        ChapterData chapter;
        chapter.id = it->second.id;
        chapter.href = assetPath;
        chapter.mediaType = it->second.mediaType;
        chapter.content = *html;
        chapter.title = extractHtmlTitle(chapter.content);
        chapters_.push_back(std::move(chapter));
    }

    if (chapters_.empty()) {
        std::cerr << "[EpubLoader] No readable chapters found" << std::endl;
        return false;
    }

    return true;
}

const EpubLoader::PackageMetadata& EpubLoader::metadata() const {
    return metadata_;
}

const EpubLoader::ChapterData* EpubLoader::getChapter(int index) const {
    if (index < 0 || index >= static_cast<int>(chapters_.size())) {
        return nullptr;
    }
    return &chapters_[static_cast<std::size_t>(index)];
}

int EpubLoader::chapterCount() const {
    return static_cast<int>(chapters_.size());
}

const std::vector<EpubLoader::ChapterData>& EpubLoader::chapters() const {
    return chapters_;
}
