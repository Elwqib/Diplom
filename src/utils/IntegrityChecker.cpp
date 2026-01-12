#include "IntegrityChecker.h"
#include <algorithm>

bool IntegrityChecker::hasMagicBytes(const fs::path& path, const std::vector<uint8_t>& bytes, size_t offset) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    file.seekg(offset);
    std::vector<uint8_t> buffer(bytes.size());
    file.read(reinterpret_cast<char*>(buffer.data()), bytes.size());
    return file.gcount() == bytes.size() && buffer == bytes;
}

bool IntegrityChecker::hasBytesAtEnd(const fs::path& path, const std::vector<uint8_t>& bytes) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;

    auto size = file.tellg();
    if (size < static_cast<std::streampos>(bytes.size())) return false;

    file.seekg(-static_cast<std::streamoff>(bytes.size()), std::ios::end);
    std::vector<uint8_t> buffer(bytes.size());
    file.read(reinterpret_cast<char*>(buffer.data()), bytes.size());
    return buffer == bytes;
}

bool IntegrityChecker::containsString(const std::string& content, const std::string& str) {
    return content.find(str) != std::string::npos;
}

void IntegrityChecker::checkJpeg(FileMetadata& meta, const fs::path& path) {
    if (!hasMagicBytes(path, {0xFF, 0xD8, 0xFF})) {
        meta.setError("Повреждённый JPEG: нет сигнатуры FF D8 FF");
        return;
    }

    if (!hasBytesAtEnd(path, {0xFF, 0xD9})) {
        meta.set("Целостность JPEG", "Предупреждение: отсутствует маркер конца (FF D9)");
    } else {
        meta.set("Целостность JPEG", "Валидный");
    }

    // Проверяем наличие APP0 (JFIF)
    if (hasMagicBytes(path, {0xFF, 0xE0}, 2) && hasMagicBytes(path, {'J', 'F', 'I', 'F'}, 6)) {
        meta.set("Стандарт", "JFIF");
    }
}

void IntegrityChecker::checkPdf(FileMetadata& meta, const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return;

    std::string content((std::istreambuf_iterator<char>(file)), {});
    file.close();

    if (content.rfind("%EOF", content.size() - 10) == std::string::npos) {
        meta.set("Целостность PDF", "Предупреждение: нет %EOF в конце");
    } else if (content.find("xref") == std::string::npos || content.find("trailer") == std::string::npos) {
        meta.set("Целостность PDF", "Предупреждение: повреждён xref/trailer");
    } else {
        meta.set("Целостность PDF", "Валидный");
    }
}

void IntegrityChecker::checkDocx(FileMetadata& meta, const fs::path& path) {
    if (!hasMagicBytes(path, {0x50, 0x4B, 0x03, 0x04})) {
        meta.setError("Не является DOCX (нет ZIP-сигнатуры)");
        return;
    }

    // Проверяем наличие обязательных файлов
    bool hasContentTypes = false, hasCore = false, hasDocument = false;

    // Это упрощённая проверка — можно улучшить через minizip, но для диплома хватит
    std::ifstream f(path, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(f)), {});
    f.close();

    hasContentTypes = containsString(content, "[Content_Types].xml");
    hasCore = containsString(content, "docProps/core.xml");
    hasDocument = containsString(content, "word/document.xml");

    if (hasContentTypes && hasCore && hasDocument) {
        meta.set("Целостность DOCX", "Полная структура");
    } else {
        std::string missing;
        if (!hasContentTypes) missing += "[Content_Types].xml ";
        if (!hasCore) missing += "docProps/core.xml ";
        if (!hasDocument) missing += "word/document.xml ";
        meta.set("Целостность DOCX", "Предупреждение: отсутствуют файлы: " + missing);
    }
}

void IntegrityChecker::checkMp3(FileMetadata& meta, const fs::path& path) {
    if (hasMagicBytes(path, {'I', 'D', '3'})) {
        meta.set("MP3 теги", "ID3v2");
    } else if (hasMagicBytes(path, {0xFF, 0xFB}) || hasMagicBytes(path, {0xFF, 0xF3}) || hasMagicBytes(path, {0xFF, 0xF2})) {
        meta.set("MP3 теги", "Без ID3 (чистый поток)");
    } else {
        meta.set("Целостность MP3", "Предупреждение: нет валидного заголовка фрейма");
    }
}

void IntegrityChecker::checkWav(FileMetadata& meta, const fs::path& path) {
    if (hasMagicBytes(path, {'R', 'I', 'F', 'F'}) && 
        hasMagicBytes(path, {'W', 'A', 'V', 'E'}, 8)) {
        meta.set("Целостность WAV", "Валидный");
    } else {
        meta.setError("Повреждённый WAV: нет RIFF/WAVE");
    }
}

void IntegrityChecker::checkPng(FileMetadata& meta, const fs::path& path) {
    if (!hasMagicBytes(path, {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'})) {
        meta.setError("Повреждённый PNG: нет сигнатуры 89 PNG");
        return;
    }

    // Проверка на наличие CRC в конце
    if (!hasBytesAtEnd(path, {0x49, 'E', 'N', 'D', 0xAE, 0x42, 0x60, 0x82})) {
        meta.set("Целостность PNG", "Предупреждение: отсутствует маркер конца (IEND)");
    } else {
        meta.set("Целостность PNG", "Валидный");
    }
}