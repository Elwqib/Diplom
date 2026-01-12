#pragma once
#ifndef PNG_ANALYZER_H
#define PNG_ANALYZER_H

#include "BaseAnalyzer.h"

class PngAnalyzer : public BaseAnalyzer {
public:
    bool canAnalyze(const fs::path& path) const override;
    FileMetadata analyze(const fs::path& path) override; 
};

#endif 