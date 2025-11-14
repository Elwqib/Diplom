#pragma once
#include "FileMetadata.h"
#include <filesystem>
namespace fs = std::filesystem;

class BaseAnalyzer {
public:
    virtual ~BaseAnalyzer() = default;
    virtual bool canAnalyze(const fs::path& path) const = 0;
    virtual FileMetadata analyze(const fs::path& path) = 0; 

protected:
    static void addBasicInfo(FileMetadata& meta, const fs::path& path);
    static std::string formatTime(const fs::file_time_type& ftime);
};