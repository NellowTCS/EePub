#include "ePubLoader.h"
#include "miniz.h"
#include <fstream>
#include <sstream>

EpubLoader::EpubLoader() {}
EpubLoader::~EpubLoader() {}

bool EpubLoader::load(const std::string& path) {
    chapters.clear();
    // Simplified: read ZIP entries for HTML files
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_file(&zip, path.c_str(), 0)) return false;

    int num_files = (int)mz_zip_reader_get_num_files(&zip);
    for (int i = 0; i < num_files; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip, i, &file_stat)) continue;

        std::string name = file_stat.m_filename;
        if (name.size() >= 5 && name.substr(name.size()-5) == ".html") {
            size_t size = (size_t)file_stat.m_uncomp_size;
            char* buf = new char[size + 1];
            if (mz_zip_reader_extract_to_mem(&zip, i, buf, size, 0)) {
                buf[size] = '\0';
                chapters.push_back(std::string(buf));
            }
            delete[] buf;
        }
    }
    mz_zip_reader_end(&zip);
    return !chapters.empty();
}

std::string EpubLoader::getChapter(int index) {
    if (index < 0 || index >= (int)chapters.size()) return "";
    return chapters[index];
}

int EpubLoader::chapterCount() const {
    return (int)chapters.size();
}
