#include <iostream>
#include <vector>
#include <memory>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <string>
#include <ctime>
#include <thread>
#include <chrono>
#include <limits>

#define NOMINMAX
#include <windows.h>

#include <nlohmann/json.hpp>

#include "analyzers/BaseAnalyzer.h"
#include "analyzers/JpegAnalyzer.h"
#include "analyzers/AudioAnalyzer.h"
#include "analyzers/PdfAnalyzer.h"
#include "analyzers/DocxAnalyzer.h"

namespace fs = std::filesystem;

// ====================================================
// НАСТРОЙКИ
// ====================================================

const std::vector<std::string> SUPPORTED_EXTENSIONS = {
    ".jpg", ".jpeg", ".pdf", ".docx",
    ".mp3", ".flac", ".wav", ".ogg", ".m4a", ".aac", ".wma"
};

enum class OutputFormat { Console, Txt, Json, Xml };
OutputFormat selectedFormat = OutputFormat::Console;

// ====================================================
// ЛОГИРОВАНИЕ
// ====================================================

void logMessage(const std::string& msg) {
    fs::create_directories("logs");
    std::ofstream log("logs/log.txt", std::ios::app);
    if (log) {
        auto t = std::time(nullptr);
        std::tm tm{};
        localtime_s(&tm, &t);
        char buf[64];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        log << "[" << buf << "] " << msg << "\n";
    }
}

// ====================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ====================================================

void trim(std::string& s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
}

void normalizePath(std::string& path) {
    trim(path);
    if (path.size() >= 2 && path.front() == '"' && path.back() == '"') {
        path = path.substr(1, path.size() - 2);
    }
    trim(path);
    std::replace(path.begin(), path.end(), '\\', '/');
}

bool isSupportedExtension(const std::string& extLower) {
    return std::find(SUPPORTED_EXTENSIONS.begin(), SUPPORTED_EXTENSIONS.end(), extLower) != SUPPORTED_EXTENSIONS.end();
}

void printSupportedExtensions() {
    system("cls");
    std::cout << "\nПоддерживаемые форматы:\n\n";
    std::cout << "Изображения: .jpg .jpeg\n";
    std::cout << "Документы:   .pdf .docx\n";
    std::cout << "Аудио:       .mp3 .flac .wav .ogg .m4a .aac .wma\n\n";
    std::cout << "Нажмите Enter...";
    std::cin.get();
}

// Безопасное variant → string
std::string variantToString(const Value& v) {
    if (const auto* s = std::get_if<std::string>(&v)) return *s;
    if (const auto* i = std::get_if<int64_t>(&v))      return std::to_string(*i);
    if (const auto* d = std::get_if<double>(&v))       return std::to_string(*d);
    if (const auto* b = std::get_if<bool>(&v))         return *b ? "true" : "false";
    return "";
}

// Manual replaceAll
void replaceAll(std::string& s, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = s.find(from, start_pos)) != std::string::npos) {
        s.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

// escapeXml
std::string escapeXml(const std::string& data) {
    std::string s = data;
    replaceAll(s, "&", "&amp;");
    replaceAll(s, "<", "&lt;");
    replaceAll(s, ">", "&gt;");
    replaceAll(s, "\"", "&quot;");
    replaceAll(s, "'", "&apos;");
    return s;
}

// Сохранение отчётов — имя на основе введённого пользователем
void saveResults(const std::vector<FileMetadata>& results,
                 const std::string& inputStr,
                 OutputFormat format,
                 const std::string& customName = "")
{
    fs::create_directories("results");

    std::string safeName;

    // Если пользователь ввёл своё имя — используем его
    if (!customName.empty()) {
        safeName = customName;
    } else {
        // Старое поведение: берём имя из введённого пути
        safeName = inputStr;
        size_t lastSlash = safeName.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            safeName = safeName.substr(lastSlash + 1);

        size_t dot = safeName.find_last_of('.');
        if (dot != std::string::npos)
            safeName = safeName.substr(0, dot);

        if (safeName.empty())
            safeName = "report";
    }

    // Чистим имя от запрещённых символов (только ASCII, UTF-8 не трогаем)
    for (char& c : safeName) {
        if (c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|')
        {
            c = '_';
        }
    }

    // Базовая папка results (ASCII — тут всё ок)
    fs::path baseDir = fs::u8path("results");

    auto makePath = [&](const std::string& ext) {
        // safeName у нас в UTF-8 → u8path сделает нормальный fs::path
        fs::path fileName = fs::u8path(safeName + ext);
        return baseDir / fileName;
    };

    switch (format) {
        case OutputFormat::Txt: {
            fs::path p = makePath(".txt");
            std::ofstream out(p, std::ios::binary);
            if (!out) break;

            const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
            out.write(reinterpret_cast<const char*>(bom), 3);

            out << "=== ОТЧЁТ ПО АНАЛИЗУ МЕТАДАННЫХ ===\n";
            out << "Дата: " << __DATE__ << " " << __TIME__ << "\n";
            out << std::string(60, '=') << "\n\n";

            if (results.empty()) {
                out << "Нет результатов анализа.\n";
            } else {
                for (const auto& m : results) {
                    out << m.toString() << "\n" << std::string(60, '-') << "\n\n";
                }
            }

            std::cout << "TXT-отчёт сохранён: " << safeName << ".txt\n";
            break;
        }
        case OutputFormat::Json: {
            fs::path p = makePath(".json");
            nlohmann::json j = nlohmann::json::array();
            for (const auto& m : results)
                j.push_back(m.toJson());
            std::ofstream out(p);
            if (!out) break;
            out << std::setw(4) << j << "\n";
            std::cout << "JSON-отчёт сохранён: " << safeName << ".json\n";
            break;
        }
        case OutputFormat::Xml: {
            fs::path p = makePath(".xml");
            std::ofstream out(p);
            if (!out) break;

            out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<Report>\n";
            for (const auto& m : results) {
                out << "  <File path=\"" << escapeXml(m.path.u8string()) << "\">\n";
                if (!m.error.empty())
                    out << "    <Error>" << escapeXml(m.error) << "</Error>\n";
                for (const auto& [k, v] : m.data) {
                    out << "    <" << escapeXml(k) << ">"
                        << escapeXml(variantToString(v))
                        << "</" << escapeXml(k) << ">\n";
                }
                out << "  </File>\n";
            }
            out << "</Report>\n";
            std::cout << "XML-отчёт сохранён: " << safeName << ".xml\n";
            break;
        }
        case OutputFormat::Console:
            break;
    }
}


// ====================================================
// MAIN
// ====================================================

int main(int argc, char* argv[]) {
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);

    logMessage("Программа запущена");

    std::vector<std::unique_ptr<BaseAnalyzer>> analyzers;
    analyzers.emplace_back(std::make_unique<JpegAnalyzer>());
    analyzers.emplace_back(std::make_unique<AudioAnalyzer>());
    analyzers.emplace_back(std::make_unique<PdfAnalyzer>());
    analyzers.emplace_back(std::make_unique<DocxAnalyzer>());

    fs::create_directories("results");

    while (true) {
        system("cls");
        std::cout << "=== АНАЛИЗАТОР МЕТАДАННЫХ v1.0 ===\n\n";
        std::cout << "1. Анализ одного файла\n";
        std::cout << "2. Анализ папки (рекурсивно)\n";
        std::cout << "3. Выход\n";
        std::cout << "4. Поддерживаемые форматы\n";
        std::cout << "5. Формат отчёта → ";
        switch (selectedFormat) {
            case OutputFormat::Console: std::cout << "Консоль"; break;
            case OutputFormat::Txt:     std::cout << "TXT";     break;
            case OutputFormat::Json:    std::cout << "JSON";    break;
            case OutputFormat::Xml:     std::cout << "XML";     break;
        }
        std::cout << "\n\nВыбор: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 3) {
            std::cout << "До свидания!\n";
            return 0;
        }
        if (choice == 4) {
            printSupportedExtensions();
            continue;
        }
        if (choice == 5) {
            std::cout << "\n1. Консоль  2. TXT  3. JSON  4. XML\nВыбор: ";
            int f;
            if (std::cin >> f && f >= 1 && f <= 4) {
                selectedFormat = static_cast<OutputFormat>(f - 1);
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        if (choice != 1 && choice != 2) {
            std::cout << "Неверный выбор!\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        std::cout << "\nВведите путь: ";
        std::string path_str;
        std::getline(std::cin, path_str);
        if (path_str.empty()) {
            std::cout << "Путь не введён\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        normalizePath(path_str);
        fs::path root = fs::u8path(path_str);

        std::vector<FileMetadata> results;

        auto processFile = [&](const fs::path& p) {
            if (!fs::is_regular_file(p)) return;

            std::string ext = p.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            if (!isSupportedExtension(ext)) {
                std::cout << "Пропущен (неподдерживаемый формат): "
                          << p.filename().u8string() << "\n";
                return;
            }

            for (const auto& a : analyzers) {
                if (a->canAnalyze(p)) {
                    FileMetadata meta = a->analyze(p);
                    results.push_back(meta);
                    std::cout << meta.toString() << "\n\n";
                    break;
                }
            }
        };

        if (choice == 2 && fs::is_directory(root)) {
            std::cout << "Рекурсивный анализ папки...\n\n";
            for (const auto& entry : fs::recursive_directory_iterator(root)) {
                processFile(entry.path());
            }
        } else {
            processFile(root);
        }

        if (selectedFormat != OutputFormat::Console) {
            std::cout << "\nВведите имя файла отчёта (без расширения).\n"
                         "Оставьте пустым, чтобы использовать имя по умолчанию: ";
            std::string reportName;
            std::getline(std::cin, reportName);

            saveResults(results, path_str, selectedFormat, reportName);
        }

        std::cout << "\nГотово! Обработано файлов: " << results.size() << "\n";
        std::cout << "Нажмите Enter для продолжения...";
        std::cin.get();
    }

    return 0;
}
