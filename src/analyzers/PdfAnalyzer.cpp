#include "PdfAnalyzer.h"
#include <fstream>
#include <regex>
#include <sstream>

bool PdfAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".pdf";
}

static std::string unescapePdfString(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '\\' && i + 1 < s.size()) {
            char n = s[++i];
            switch (n) {
                case 'n':  out.push_back('\n'); break;
                case 'r':  out.push_back('\r'); break;
                case 't':  out.push_back('\t'); break;
                case 'b':  out.push_back('\b'); break;
                case 'f':  out.push_back('\f'); break;
                case '\\': out.push_back('\\'); break;
                case '(':  out.push_back('(');  break;
                case ')':  out.push_back(')');  break;
                default:   out.push_back(n);    break;
            }
        } else {
            out.push_back(c);
        }
    }
    return out;
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

    // Базовые метаданные
    std::regex r(R"(/(Author|Creator|Title|CreationDate|ModDate|Producer)\s*\(([^()]*)\))");
    std::sregex_iterator iter(content.begin(), content.end(), r);
    std::sregex_iterator end;

    for (; iter != end; ++iter) {
        std::string key = (*iter)[1].str();
        std::string val = (*iter)[2].str();

        if (key == "Author")       meta.set("Автор",          val);
        else if (key == "Creator") meta.set("Создатель",      val);
        else if (key == "Title")   meta.set("Заголовок",      val);
        else if (key == "CreationDate") meta.set("Дата создания",   val);
        else if (key == "ModDate")      meta.set("Дата изменения", val);
        else if (key == "Producer")     meta.set("Программа",      val);
    }

    meta.set("Защищён паролем", content.find("/Encrypt") != std::string::npos);

    // Проверка целостности — наличие маркера EOF
    if (content.rfind("%%EOF") == std::string::npos) {
        meta.set("Файл повреждён (нет %%EOF)", true);
    }

    // Простейшее извлечение текста: строки в операторах Tj/TJ
    try {
        std::regex textRe(R"(\(([^()]*)\)\s*T[Jj])");
        std::sregex_iterator tit(content.begin(), content.end(), textRe);
        std::sregex_iterator tend;

        std::string text;
        for (; tit != tend; ++tit) {
            std::string frag = (*tit)[1].str();
            text += unescapePdfString(frag);
            text += " ";
            if (text.size() > 8000) break; // ограничим
        }

        if (!text.empty()) {
            std::string snippet = text;
            if (snippet.size() > 4000) snippet = snippet.substr(0, 4000) + "... (обрезано)";
            meta.set("Извлечённый текст (фрагмент)", snippet);
            meta.set("Длина извлечённого текста (символы)", static_cast<int64_t>(text.size()));
        }
    } catch (...) {
        // игнорируем ошибки при парсинге текста
    }

    return meta;
}
