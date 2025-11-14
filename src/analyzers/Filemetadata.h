#pragma once
#include <string>
#include <map>
#include <variant>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <sstream>

namespace fs = std::filesystem;

using Value = std::variant<std::string, int64_t, double, bool>;

class FileMetadata {
public:
    fs::path path;
    std::map<std::string, Value> data;
    std::string error;

    FileMetadata(const fs::path& p) : path(p) {}

    template<typename T>
    void set(const std::string& key, const T& value) {
        data[key] = value;
    }

    void setError(const std::string& msg) {
        error = msg;
    }

    bool hasError() const { return !error.empty(); }

    nlohmann::json toJson() const {
        nlohmann::json j;
        j["file"] = path.string();
        j["error"] = error;

        nlohmann::json meta;
        for (const auto& [k, v] : data) {
            std::visit([&meta, k](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>)
                    meta[k] = arg;
                else if constexpr (std::is_same_v<T, int64_t>)
                    meta[k] = arg;
                else if constexpr (std::is_same_v<T, double>)
                    meta[k] = arg;
                else if constexpr (std::is_same_v<T, bool>)
                    meta[k] = arg;
            }, v);
        }
        j["metadata"] = meta;
        return j;
    }

    std::string toString() const {
        std::ostringstream oss;
        oss << "Файл: " << path.filename().string() << "\n";
        if (!error.empty()) {
            oss << "ОШИБКА: " << error << "\n";
        }

        for (const auto& [k, v] : data) {
            oss << k << ": ";
            std::visit([&oss](const auto& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>)
                    oss << arg;
                else if constexpr (std::is_same_v<T, int64_t>)
                    oss << arg;
                else if constexpr (std::is_same_v<T, double>)
                    oss << arg;
                else if constexpr (std::is_same_v<T, bool>)
                    oss << (arg ? "true" : "false");
            }, v);
            oss << "\n";
        }
        oss << "\n";
        return oss.str();
    }
};