#include "PsdAnalyzer.h"
#include <fstream>
#include <algorithm>

namespace {
    uint16_t readBE16(std::ifstream& in) {
        unsigned char b[2];
        in.read(reinterpret_cast<char*>(b), 2);
        return static_cast<uint16_t>((b[0] << 8) | b[1]);
    }

    uint32_t readBE32(std::ifstream& in) {
        unsigned char b[4];
        in.read(reinterpret_cast<char*>(b), 4);
        return (static_cast<uint32_t>(b[0]) << 24) |
               (static_cast<uint32_t>(b[1]) << 16) |
               (static_cast<uint32_t>(b[2]) << 8)  |
               (static_cast<uint32_t>(b[3]));
    }
}

bool PsdAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".psd";
}

FileMetadata PsdAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        meta.setError("Не удалось открыть PSD-файл");
        return meta;
    }

    char signature[4];
    in.read(signature, 4);
    if (!in || std::string(signature, 4) != "8BPS") {
        meta.setError("Не является PSD-файлом");
        return meta;
    }

    uint16_t version = readBE16(in);
    meta.set("Версия PSD", static_cast<int64_t>(version));

    char reserved[6];
    in.read(reserved, 6);

    uint16_t channels = readBE16(in);
    uint32_t height   = readBE32(in);
    uint32_t width    = readBE32(in);
    uint16_t depth    = readBE16(in);
    uint16_t colorMode= readBE16(in);

    meta.set("Ширина",  static_cast<int64_t>(width));
    meta.set("Высота",  static_cast<int64_t>(height));
    meta.set("Каналы",  static_cast<int64_t>(channels));
    meta.set("Глубина цвета", static_cast<int64_t>(depth));
    meta.set("Режим цвета (код)", static_cast<int64_t>(colorMode));

    if (channels > 8 || depth > 32) {
        meta.set("Подозрение: нестандартные параметры PSD", true);
    }

    return meta;
}
