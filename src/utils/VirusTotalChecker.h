#pragma once
#include <string>
#include <filesystem>
#include "FileMetadata.h"

namespace fs = std::filesystem;

class VirusTotalChecker {
public:
    // apiKey передаём извне, чтобы его не хардкодить в коде
    static void analyzeFile(FileMetadata& meta, const fs::path& path, const std::string& apiKey);

private:
    static std::string calculateSha256(const fs::path& path);
    static std::string queryVirusTotal(const std::string& sha256, const std::string& apiKey);
};
