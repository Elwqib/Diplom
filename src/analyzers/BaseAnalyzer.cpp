#include "BaseAnalyzer.h"
#include "utils/IntegrityChecker.h"
#include "utils/VirusTotalChecker.h"


#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <algorithm> 
#include <cstdint>   

std::string BaseAnalyzer::vtApiKey;

std::string BaseAnalyzer::formatTime(const fs::file_time_type& ftime) {
    try {
        auto sys_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        auto tt = std::chrono::system_clock::to_time_t(sys_time);
        std::ostringstream oss;
        oss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
        return oss.str();
    } catch (...) {
        return "Неизвестно";
    }
}

void BaseAnalyzer::addIntegrityInfo(FileMetadata& meta, const fs::path& path) {
    try {
        std::ifstream in(path, std::ios::binary);
        if (!in) return;

        uint32_t crc = 0xFFFFFFFFu;
        char buffer[4096];

        auto updateCrc = [&crc](const char* data, std::streamsize len) {
            for (std::streamsize i = 0; i < len; ++i) {
                uint8_t byte = static_cast<uint8_t>(data[i]);
                crc ^= byte;
                for (int j = 0; j < 8; ++j) {
                    uint32_t mask = -(crc & 1u);
                    crc = (crc >> 1) ^ (0xEDB88320u & mask);
                }
            }
        };

        while (in) {
            in.read(buffer, sizeof(buffer));
            std::streamsize got = in.gcount();
            if (got > 0) updateCrc(buffer, got);
        }

        crc ^= 0xFFFFFFFFu;

        std::ostringstream oss;
        oss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << crc;
        meta.set("CRC32", oss.str());
    } catch (...) {
        // игнорируем ошибки при подсчёте
    }
}

void BaseAnalyzer::addBasicInfo(FileMetadata& meta, const fs::path& path) {
    try {
        meta.set("Полный путь", path.u8string());
        meta.set("Имя файла", path.filename().u8string());
        meta.set("Размер (байт)", static_cast<int64_t>(fs::file_size(path)));
        meta.set("Изменён", formatTime(fs::last_write_time(path)));
        meta.set("Расширение", path.extension().u8string());

        // Глобальная проверка сигнатур
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".jpg" || ext == ".jpeg")      IntegrityChecker::checkJpeg(meta, path);
        else if (ext == ".pdf")                   IntegrityChecker::checkPdf(meta, path);
        else if (ext == ".docx")                  IntegrityChecker::checkDocx(meta, path);
        else if (ext == ".mp3")                   IntegrityChecker::checkMp3(meta, path);
        else if (ext == ".wav")                   IntegrityChecker::checkWav(meta, path);
        else if (ext == ".png")                   IntegrityChecker::checkPng(meta, path);

        addIntegrityInfo(meta, path);

        if (!vtApiKey.empty()) {
            VirusTotalChecker::analyzeFile(meta, path, vtApiKey);
        }
        else {
            meta.set("VirusTotal", "API ключ не задан");
        }


    } catch (...) {
        meta.setError("Файл недоступен или повреждён");
    }
}


void BaseAnalyzer::setVirusTotalApiKey(const std::string& key) {
    vtApiKey = key;
}
