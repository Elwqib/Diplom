#include "DocxAnalyzer.h"
#include <fstream>

bool DocxAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    return ext == ".docx" || ext == ".DOCX";
}

FileMetadata DocxAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        meta.setError("Не удалось открыть файл");
        return meta;
    }

    char sig[4] = {};
    file.read(sig, 4);
    if (file.gcount() < 4 || sig[0] != 'P' || sig[1] != 'K') {
        meta.setError("Не является ZIP-архивом (не DOCX)");
        return meta;
    }

    meta.set("Формат", "Valid DOCX (ZIP-архив)");
    meta.set("Статус", "OK");

    return meta;
}