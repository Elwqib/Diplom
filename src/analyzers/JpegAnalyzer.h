#pragma once
#include "BaseAnalyzer.h"

class JpegAnalyzer : public BaseAnalyzer {
public:
    bool canAnalyze(const fs::path& path) const override;
    FileMetadata analyze(const fs::path& path) override; 
};