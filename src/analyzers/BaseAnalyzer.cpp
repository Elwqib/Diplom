#include "BaseAnalyzer.h"
#include <chrono>
#include <iomanip>
#include <sstream>

void BaseAnalyzer::addBasicInfo(FileMetadata& meta, const fs::path& path) {
    meta.data["File"] = path.filename().string();
    meta.data["Path"] = path.string();
    meta.data["Size"] = fs::file_size(path);

    auto last_write = fs::last_write_time(path);
    std::time_t tt = static_cast<std::time_t>(last_write);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S");
    meta.data["Modified"] = ss.str();
}