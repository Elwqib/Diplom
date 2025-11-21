#pragma once

#include "BaseAnalyzer.h"
#include <filesystem>

namespace fs = std::filesystem;

class DocxAnalyzer : public BaseAnalyzer {
public:
    DocxAnalyzer() = default;
    ~DocxAnalyzer() override = default;

    bool canAnalyze(const fs::path& path) const override;
    FileMetadata analyze(const fs::path& path) override;
};
