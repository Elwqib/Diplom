#include "BaseAnalyzer.h"
#include <sstream>
#include <iomanip>
#include <chrono>

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

void BaseAnalyzer::addBasicInfo(FileMetadata& meta, const fs::path& path) {
    try {
        meta.set("Полный путь", path.string());
        meta.set("Имя файла", path.filename().string());
        meta.set("Размер (байт)", static_cast<int64_t>(fs::file_size(path)));
        meta.set("Изменён", formatTime(fs::last_write_time(path)));
        meta.set("Расширение", path.extension().string());
    } catch (...) {
        meta.setError("Файл недоступен или повреждён");
    }
}