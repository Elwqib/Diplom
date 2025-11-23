#include "Mp4Analyzer.h"
#include <fstream>
#include <algorithm>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <sstream>

namespace {
    uint32_t readBE32(std::ifstream& in) {
        unsigned char b[4];
        in.read(reinterpret_cast<char*>(b), 4);
        return (static_cast<uint32_t>(b[0]) << 24) |
               (static_cast<uint32_t>(b[1]) << 16) |
               (static_cast<uint32_t>(b[2]) << 8)  |
               (static_cast<uint32_t>(b[3]));
    }
}

bool Mp4Analyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".mp4" || ext == ".m4v" || ext == ".mov";
}

FileMetadata Mp4Analyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    meta.set("Формат контейнера", "ISO Base Media (MP4/MOV)");

    // Разбор бокса ftyp
    std::ifstream in(path, std::ios::binary);
    if (in) {
        uint32_t size = readBE32(in);
        char type[4];
        in.read(type, 4);
        if (in && std::string(type, 4) == "ftyp") {
            char majorBrand[4];
            in.read(majorBrand, 4);
            uint32_t minor = readBE32(in);

            meta.set("Major brand", std::string(majorBrand, 4));
            meta.set("Minor version", static_cast<int64_t>(minor));

            std::ostringstream brands;
            while (in.tellg() < std::streampos(size)) {
                char compat[4];
                in.read(compat, 4);
                if (!in) break;
                brands << std::string(compat, 4) << " ";
            }
            std::string b = brands.str();
            if (!b.empty())
                meta.set("Compatible brands", b);
        }
    }

    // Теги через TagLib
    TagLib::FileRef f(path.wstring().c_str());
    if (!f.isNull() && f.tag()) {
        TagLib::Tag* tag = f.tag();
        if (!tag->title().isEmpty())   meta.set("Название",   tag->title().toCString(true));
        if (!tag->artist().isEmpty())  meta.set("Исполнитель",tag->artist().toCString(true));
        if (!tag->album().isEmpty())   meta.set("Альбом",     tag->album().toCString(true));
        if (!tag->genre().isEmpty())   meta.set("Жанр",       tag->genre().toCString(true));
        if (tag->year() > 0)           meta.set("Год",        static_cast<int64_t>(tag->year()));

        if (f.audioProperties()) {
            auto* p = f.audioProperties();
            meta.set("Длительность (сек)", static_cast<int64_t>(p->lengthInSeconds()));
            meta.set("Битрейт (kbps)",     static_cast<int64_t>(p->bitrate()));
        }

        if (auto* file = f.file()) {
            TagLib::PropertyMap props = file->properties();
            if (!props.isEmpty()) {
                std::ostringstream oss;
                for (auto it = props.begin(); it != props.end(); ++it) {
                    oss << it->first.toCString(true) << " = ";
                    const TagLib::StringList& lst = it->second;
                    bool first = true;
                    for (const auto& s : lst) {
                        if (!first) oss << ", ";
                        first = false;
                        oss << s.toCString(true);
                    }
                    oss << "\n";
                }
                std::string dump = oss.str();
                if (!dump.empty()) {
                    if (dump.size() > 4000) dump = dump.substr(0, 4000) + "... (обрезано)";
                    meta.set("Теги контейнера (PropertyMap)", dump);
                }
            }
        }
    } else {
        meta.set("Теги", "Не удалось прочитать через TagLib");
    }

    return meta;
}
