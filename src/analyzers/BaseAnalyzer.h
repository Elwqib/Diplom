#pragma once
#include <nlohmann/json.hpp>
#include <boost/filesystem.hpp>

using json = nlohmann::json;
namespace fs = boost::filesystem;

struct FileMetadata { json data; };

class BaseAnalyzer {
public:
    virtual ~BaseAnalyzer() = default;
    virtual bool canAnalyze(const fs::path& path) const = 0;
    virtual FileMetadata analyze(const fs::path& path) = 0;
protected:
    void addBasicInfo(FileMetadata& meta, const fs::path& path);
};