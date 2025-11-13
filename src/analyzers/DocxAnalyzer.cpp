#include "DocxAnalyzer.h"
#include <fstream>

bool DocxAnalyzer::canAnalyze(const fs::path& path) const {
    return path.extension().string() == ".docx";
}

FileMetadata DocxAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta;
    addBasicInfo(meta, path);

    std::ifstream file(path.string(), std::ios::binary);
    if (!file) {
        meta.data["DOCX"] = "Failed to open file";
        return meta;
    }

    char signature[2] = {0};
    file.read(signature, 2);
    if (file.gcount() != 2 || signature[0] != 'P' || signature[1] != 'K') {
        meta.data["DOCX"] = "Not a ZIP archive";
        return meta;
    }

    meta.data["DOCX"] = "Valid DOCX (ZIP archive)";
    return meta;
}