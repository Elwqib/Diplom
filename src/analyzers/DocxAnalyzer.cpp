#include "DocxAnalyzer.h"
#include <fstream>

bool DocxAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".docx";
}

static std::string extract(const std::string& data, const std::string& tag) {
    std::string open = "<" + tag;
    std::string close = "</" + tag + ">";
    size_t s = data.find(open);
    if (s == std::string::npos) return "";
    s = data.find(">", s);
    if (s == std::string::npos) return "";
    s++;
    size_t e = data.find(close, s);
    if (e == std::string::npos) return "";
    return data.substr(s, e - s);
}

FileMetadata DocxAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    std::ifstream f(path, std::ios::binary);
    if (!f) {
        meta.setError("Не удалось открыть файл");
        return meta;
    }

    std::string content((std::istreambuf_iterator<char>(f)), {});

    if (content.find("PK") != 0) {
        meta.setError("Не является DOCX");
        return meta;
    }

    meta.set("Формат", "Office Open XML (DOCX)");
    meta.set("Статус", "Валидный");

    // Извлечение полей
    meta.set("Автор", extract(content, "dc:creator"));
    meta.set("Дата изменения", extract(content, "dcterms:modified"));
    meta.set("Дата создания", extract(content, "dcterms:created"));
    meta.set("Компания", extract(content, "Company"));
    meta.set("Программа", extract(content, "Application"));
    meta.set("Ревизия", extract(content, "cp:revision"));
    meta.set("Страниц", extract(content, "Pages"));

    return meta;
}