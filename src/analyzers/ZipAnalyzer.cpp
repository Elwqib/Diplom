#include "ZipAnalyzer.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <string>

bool ZipAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".zip";
}

static bool endsWith(const std::string& s, const std::string& suffix) {
    if (s.size() < suffix.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), s.rbegin());
}

FileMetadata ZipAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        meta.setError("Не удалось открыть ZIP-файл");
        return meta;
    }

    // Проверим сигнатуру в начале файла: "PK\003\004"
    char sig[4] = {0};
    in.read(sig, 4);
    if (!in || sig[0] != 'P' || sig[1] != 'K') {
        meta.setError("Не является ZIP-архивом (нет сигнатуры PK)");
        return meta;
    }

    meta.set("Формат", "ZIP-архив");

    // Начинаем разбор с начала файла
    in.clear();
    in.seekg(0, std::ios::beg);

    std::size_t fileCount = 0;
    std::uint64_t totalCompressed = 0;
    std::uint64_t totalUncompressed = 0;
    bool suspiciousExec = false;

    while (true) {
        // Читаем сигнатуру следующего блока
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

            // Сохраним имя файла
            meta.set("Файл #" + std::to_string(fileCount + 1), name);

            // Проверка на подозрительные расширения внутри ZIP
            std::string lowerName = name;
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

            if (endsWith(lowerName, ".exe") ||
                endsWith(lowerName, ".dll") ||
                endsWith(lowerName, ".js")  ||
                endsWith(lowerName, ".vbs") ||
                endsWith(lowerName, ".bat") ||
                endsWith(lowerName, ".cmd"))
            {
                suspiciousExec = true;
            }

            ++fileCount;
            totalCompressed   += compSize;
            totalUncompressed += uncompSize;

            // Пропускаем тело файла (сжатые данные)
            if (compSize > 0) {
                in.seekg(static_cast<std::streamoff>(compSize), std::ios::cur);
            }
        }
        // Central directory header: "PK\001\002" или End of central dir: "PK\005\006"
        else if ((headerSig[0] == 'P' && headerSig[1] == 'K' &&
                  headerSig[2] == 0x01 && headerSig[3] == 0x02) ||
                 (headerSig[0] == 'P' && headerSig[1] == 'K' &&
                  headerSig[2] == 0x05 && headerSig[3] == 0x06))
        {
            // Центральный каталог / конец каталога — дальше можно не разбирать
            break;
        }
        else {
            // Неожиданная подпись — выходим из цикла, чтобы не зациклиться
            break;
        }
    }

    meta.set("Количество файлов в ZIP", static_cast<int64_t>(fileCount));
    meta.set("Сжатый размер (суммарно)", static_cast<int64_t>(totalCompressed));
    meta.set("Несжатый размер (суммарно)", static_cast<int64_t>(totalUncompressed));

    if (totalCompressed > 0 && totalUncompressed > 0) {
        double ratio = 100.0 - (static_cast<double>(totalCompressed) /
                                static_cast<double>(totalUncompressed) * 100.0);
        meta.set("Степень сжатия (%)", ratio);
    }

    if (suspiciousExec) {
        meta.set("Подозрение: исполняемые файлы внутри ZIP", true);
    }

    return meta;
}
