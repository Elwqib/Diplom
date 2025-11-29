#include "VirusTotalChecker.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

#include <openssl/sha.h>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using nlohmann::json;

std::string VirusTotalChecker::calculateSha256(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};

    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    std::vector<char> buffer(8192);
    while (file) {
        file.read(buffer.data(), buffer.size());
        std::streamsize readBytes = file.gcount();
        if (readBytes > 0) {
            SHA256_Update(&ctx, buffer.data(), static_cast<size_t>(readBytes));
        }
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &ctx);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::nouppercase;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return oss.str();
}

std::string VirusTotalChecker::queryVirusTotal(const std::string& sha256, const std::string& apiKey) {
    if (sha256.empty()) {
        return "Не удалось вычислить SHA-256";
    }

    if (apiKey.empty()) {
        return "API ключ VirusTotal не задан";
    }

    std::string url = "https://www.virustotal.com/api/v3/files/" + sha256;

    cpr::Response r = cpr::Get(
        cpr::Url{url},
        cpr::Header{{"x-apikey", apiKey}}
    );

    if (r.error) {
        return "Ошибка HTTP: " + r.error.message;
    }

    if (r.status_code == 404) {
        return "Файл отсутствует в базе VirusTotal";
    }

    if (r.status_code != 200) {
        return "Ошибка VirusTotal: HTTP " + std::to_string(r.status_code);
    }

    try {
        json j = json::parse(r.text);

        auto stats = j["data"]["attributes"]["last_analysis_stats"];
        int malicious  = stats.value("malicious", 0);
        int suspicious = stats.value("suspicious", 0);
        int undetected = stats.value("undetected", 0);
        int harmless   = stats.value("harmless", 0);

        std::ostringstream oss;
        oss << "Вредоностный: "  << malicious
            << ", Подозрительный: " << suspicious
            << ", Безвредный: "   << harmless
            << ", Необнаруженный: " << undetected;

        return oss.str();
    } catch (...) {
        return "Ошибка разбора ответа VirusTotal";
    }
}

void VirusTotalChecker::analyzeFile(FileMetadata& meta, const fs::path& path, const std::string& apiKey) {
    try {
        std::string sha256 = calculateSha256(path);
        if (!sha256.empty()) {
            meta.set("SHA-256", sha256);
        } else {
            meta.set("SHA-256", "Не удалось вычислить");
        }

        std::string vtResult = queryVirusTotal(sha256, apiKey);
        meta.set("VirusTotal", vtResult);
    } catch (...) {
        meta.set("VirusTotal", "Ошибка при запросе к VirusTotal");
    }
}
