#include "JpegAnalyzer.h"
#include <fstream>

bool JpegAnalyzer::canAnalyze(const fs::path& path) const {
    auto ext = path.extension().string();
    return ext == ".jpg" || ext == ".jpeg";
}

FileMetadata JpegAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta;
    addBasicInfo(meta, path);

    std::ifstream file(path.string(), std::ios::binary);
    if (!file) {
        meta.data["JPEG"] = "Failed to open file";
        return meta;
    }

    char signature[2] = {0};
    file.read(signature, 2);
    if (file.gcount() != 2 || signature[0] != (char)0xFF || signature[1] != (char)0xD8) {
        meta.data["JPEG"] = "Not a JPEG file";
        return meta;
    }

    meta.data["JPEG"] = "Valid JPEG";
    return meta;
}