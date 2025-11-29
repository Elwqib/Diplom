#include "DocxAnalyzer.h"

#include <fstream>
#include <algorithm>
#include <cstdint>
#include <vector>

bool DocxAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".docx";
}

FileMetadata DocxAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    meta.set("Формат", "Office Open XML (DOCX, ZIP-контейнер)");

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        meta.setError("Не удалось открыть DOCX-файл");
        return meta;
    }

    // Проверим, что это вообще ZIP: сигнатура "PK\003\004"
    char sig[4] = {0};
    in.read(sig, 4);
    if (!in || sig[0] != 'P' || sig[1] != 'K') {
        meta.setError("DOCX должен быть ZIP-архивом (сигнатура PK не найдена)");
        return meta;
    }

    // Начинаем разбор с начала файла
    in.clear();
    in.seekg(0, std::ios::beg);

    std::size_t fileCount = 0;
    std::uint64_t totalCompressed = 0;
    std::uint64_t totalUncompressed = 0;

    bool hasContentTypes = false;
    bool hasDocumentXml  = false;
    bool hasCoreProps    = false;

    std::vector<std::string> partNames;

    while (true) {
        char headerSig[4] = {0};
        in.read(headerSig, 4);
        if (!in) break;

        // Local file header: "PK\003\004"
        if (headerSig[0] == 'P' && headerSig[1] == 'K' &&
            headerSig[2] == 0x03 && headerSig[3] == 0x04)
        {
            std::uint16_t version = 0;
            std::uint16_t flags = 0;
            std::uint16_t method = 0;
            std::uint16_t modTime = 0;
            std::uint16_t modDate = 0;
            std::uint32_t crc32 = 0;
            std::uint32_t compSize = 0;
            std::uint32_t uncompSize = 0;
            std::uint16_t nameLen = 0;
            std::uint16_t extraLen = 0;

            in.read(reinterpret_cast<char*>(&version),   sizeof(version));
            in.read(reinterpret_cast<char*>(&flags),     sizeof(flags));
            in.read(reinterpret_cast<char*>(&method),    sizeof(method));
            in.read(reinterpret_cast<char*>(&modTime),   sizeof(modTime));
            in.read(reinterpret_cast<char*>(&modDate),   sizeof(modDate));
            in.read(reinterpret_cast<char*>(&crc32),     sizeof(crc32));
            in.read(reinterpret_cast<char*>(&compSize),  sizeof(compSize));
            in.read(reinterpret_cast<char*>(&uncompSize),sizeof(uncompSize));
            in.read(reinterpret_cast<char*>(&nameLen),   sizeof(nameLen));
            in.read(reinterpret_cast<char*>(&extraLen),  sizeof(extraLen));

            if (!in) break;

            std::string name;
            name.resize(nameLen);
            if (nameLen > 0) {
                in.read(&name[0], nameLen);
            }

            // Пропускаем extra field
            if (extraLen > 0) {
                in.seekg(extraLen, std::ios::cur);
            }

            ++fileCount;
            totalCompressed   += compSize;
            totalUncompressed += uncompSize;

            // запомним имя части
            partNames.push_back(name);

            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(),
                           lowerName.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (lowerName == "[content_types].xml")
                hasContentTypes = true;
            if (lowerName == "word/document.xml")
                hasDocumentXml = true;
            if (lowerName == "docprops/core.xml")
                hasCoreProps = true;

            // пропускаем тело файла (сжатые данные)
            if (compSize > 0) {
                in.seekg(static_cast<std::streamoff>(compSize), std::ios::cur);
            }
        }
        // Central directory header или конец каталога — останавливаемся
        else if ((headerSig[0] == 'P' && headerSig[1] == 'K' &&
                  headerSig[2] == 0x01 && headerSig[3] == 0x02) ||
                 (headerSig[0] == 'P' && headerSig[1] == 'K' &&
                  headerSig[2] == 0x05 && headerSig[3] == 0x06))
        {
            break;
        }
        else {
            // что-то непонятное — выходим, чтобы не зациклиться
            break;
        }
    }

    meta.set("Количество частей (файлов внутри DOCX)", static_cast<int64_t>(fileCount));
    meta.set("Суммарный сжатый размер (байты)", static_cast<int64_t>(totalCompressed));
    meta.set("Суммарный несжатый размер (байты)", static_cast<int64_t>(totalUncompressed));

    // Список первых N частей, чтобы не раздувать отчёт
    const std::size_t MAX_LIST = 30;
    std::string list;
    for (std::size_t i = 0; i < partNames.size() && i < MAX_LIST; ++i) {
        list += partNames[i] + "\n";
    }
    if (!list.empty()) {
        if (partNames.size() > MAX_LIST) {
            list += "... (обрезано, всего частей: " +
                    std::to_string(partNames.size()) + ")";
        }
        meta.set("Список частей внутри DOCX", list);
    }

    // Простейшая проверка "валидности" структуры DOCX
    if (hasContentTypes && hasDocumentXml) {
        meta.set("Структура DOCX", "Основные части присутствуют ([Content_Types].xml, word/document.xml)");
    } else {
        meta.set("Структура DOCX", "Подозрительная: отсутствуют некоторые стандартные части");
    }

    if (hasCoreProps) {
        meta.set("Метаданные документа", "Файл содержит docProps/core.xml (основные свойства документа)");
    } else {
        meta.set("Метаданные документа", "core.xml (основные свойства документа) не найден");
    }

    // В этом варианте текст не извлекаем, об этом можно честно написать в отчёте
    meta.set("Извлечение текста",
             "В базовой конфигурации текст DOCX не извлекается (анализируется только структура контейнера). "
             "В расширенной конфигурации используется отдельная библиотека для полнотекстового анализа.");

    return meta;
}