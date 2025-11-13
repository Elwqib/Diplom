#include "PdfAnalyzer.h"
#include <fstream>
#include <sstream>

bool PdfAnalyzer::canAnalyze(const fs::path& path) const {
    return path.extension().string() == ".pdf";
}

FileMetadata PdfAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta;
    addBasicInfo(meta, path);

    std::ifstream file(path.string(), std::ios::binary);
    if (!file) {
        meta.data["PDF"] = "Failed to open file";
        return meta;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    json info;
    size_t pos = content.find("/Title");
    if (pos != std::string::npos) {
        size_t start = content.find("(", pos);
        size_t end = content.find(")", start);
        if (start != std::string::npos && end != std::string::npos) {
            info["Title"] = content.substr(start + 1, end - start - 1);
        }
    }

    pos = content.find("/Author");
    if (pos != std::string::npos) {
        size_t start = content.find("(", pos);
        size_t end = content.find(")", start);
        if (start != std::string::npos && end != std::string::npos) {
            info["Author"] = content.substr(start + 1, end - start - 1);
        }
    }

    // Count pages (simplified)
    int pages = 0;
    pos = 0;
    while ((pos = content.find("/Type /Page", pos)) != std::string::npos) {
        pages++;
        pos += 11;
    }
    info["Pages"] = pages > 0 ? pages : 1;

    info["FileSize"] = content.size();
    meta.data["PDF"] = info;
    return meta;
}