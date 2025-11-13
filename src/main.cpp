#include <iostream>
#include <vector>
#include <memory>
#include <boost/filesystem.hpp>
#include <windows.h>
#include "analyzers/BaseAnalyzer.h"
#include "analyzers/JpegAnalyzer.h"
#include "analyzers/AudioAnalyzer.h"
#include "analyzers/PdfAnalyzer.h"
#include "analyzers/DocxAnalyzer.h"
#include "utils/JsonPrinter.h"

namespace fs = boost::filesystem;

std::vector<FileMetadata> allResults;
std::vector<std::unique_ptr<BaseAnalyzer>> analyzers;

int main() {
    SetConsoleOutputCP(CP_UTF8);

    std::vector<std::unique_ptr<BaseAnalyzer>> analyzers;
    analyzers.emplace_back(std::make_unique<JpegAnalyzer>());
    analyzers.emplace_back(std::make_unique<AudioAnalyzer>());
    analyzers.emplace_back(std::make_unique<PdfAnalyzer>());
    analyzers.emplace_back(std::make_unique<DocxAnalyzer>());

    std::string input;
    int choice;

    while (true) {
        std::cout << "\n=== FILE METADATA ANALYZER ===\n";
        std::cout << "1. Analyze single file\n";
        std::cout << "2. Analyze folder (recursive)\n";
        std::cout << "3. Exit\n";
        std::cout << "Select action: ";
        std::cin >> choice;
        std::cin.ignore();

        if (choice == 3) break;
        if (choice != 1 && choice != 2) {
            std::cout << "Invalid choice!\n";
            continue;
        }

        std::cout << "Enter path: ";
        std::getline(std::cin, input);
        fs::path path(input);
        if (!fs::exists(path)) {
            std::cout << "Path does not exist!\n";
            continue;
        }

        std::vector<fs::path> files;
        if (choice == 1) {
            files.push_back(path);
        } else {
            for (const auto& entry : fs::recursive_directory_iterator(path)) {
                if (entry.is_regular_file()) files.push_back(entry.path());
            }
        }

        int analyzed = 0;
        for (const auto& file : files) {
            for (const auto& analyzer : analyzers) {
                if (analyzer->canAnalyze(file)) {
                    auto result = analyzer->analyze(file);
                    std::cout << "\n--- " << file.filename().string() << " ---\n";
                    printJson(result.data);
                    analyzed++;
                    break;
                }
            }
        }
        std::cout << "\nAnalyzed files: " << analyzed << "\n";
    }

    std::cout << "Program terminated.\n";
    return 0;
}