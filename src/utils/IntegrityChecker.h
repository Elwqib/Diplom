#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include "FileMetadata.h"

namespace fs = std::filesystem;

class IntegrityChecker {
public:
    static void checkJpeg(FileMetadata& meta, const fs::path& path);
    static void checkPdf(FileMetadata& meta, const fs::path& path);
    static void checkDocx(FileMetadata& meta, const fs::path& path);
    static void checkMp3(FileMetadata& meta, const fs::path& path);
    static void checkWav(FileMetadata& meta, const fs::path& path);

private:
    static bool hasMagicBytes(const fs::path& path, const std::vector<uint8_t>& bytes, size_t offset = 0);
    static bool hasBytesAtEnd(const fs::path& path, const std::vector<uint8_t>& bytes);
    static bool containsString(const std::string& content, const std::string& str);
};