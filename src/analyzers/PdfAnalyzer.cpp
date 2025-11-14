#include "PdfAnalyzer.h"
#include <fstream>
#include <regex>
#include <sstream>

bool PdfAnalyzer::canAnalyze(const fs::path& path) const {
    return path.extension().string() == ".pdf";
}

FileMetadata PdfAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        meta.setError("Не удалось открыть файл");
        return meta;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    if (content.find("%PDF") != 0) {
        meta.setError("Не является PDF-файлом");
        return meta;
    }

    std::regex r(R"((/(Author|Creator|Title|CreationDate|ModDate|Producer)\s*\(([^)]+)\)))");
    std::sregex_iterator iter(content.begin(), content.end(), r);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
        std::string key = (*iter)[1].str();
        std::string val = (*iter)[2].str();

        if (key == "Author") meta.set("Автор", val);
        else if (key == "Creator") meta.set("Создатель", val);
        else if (key == "Title") meta.set("Заголовок", val);
        else if (key == "CreationDate") meta.set("Дата создания", val);
        else if (key == "ModDate") meta.set("Дата изменения", val);
        else if (key == "Producer") meta.set("Программа", val);
    }

    meta.set("Защищён паролем", content.find("/Encrypt") != std::string::npos);

    return meta;
}