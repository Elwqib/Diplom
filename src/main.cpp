// src/main.cpp
#include <iostream>
#include <vector>
#include <memory>
#include <filesystem>
#include <windows.h>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <limits>
#include <nlohmann/json.hpp>

#include "analyzers/BaseAnalyzer.h"
#include "analyzers/JpegAnalyzer.h"
#include "analyzers/AudioAnalyzer.h"
#include "analyzers/PdfAnalyzer.h"
#include "analyzers/DocxAnalyzer.h"

namespace fs = std::filesystem;

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::vector<std::unique_ptr<BaseAnalyzer>> analyzers;
    analyzers.emplace_back(std::make_unique<JpegAnalyzer>());
    analyzers.emplace_back(std::make_unique<AudioAnalyzer>());
    analyzers.emplace_back(std::make_unique<PdfAnalyzer>());
    analyzers.emplace_back(std::make_unique<DocxAnalyzer>());

    while (true) {
        system("cls");
        std::cout << "=== АНАЛИЗАТОР МЕТАДАННЫХ v0.0.3 ===\n\n";
        std::cout << "1. Анализ одного файла\n";
        std::cout << "2. Анализ папки (рекурсивно)\n";
        std::cout << "3. Выход\n\n";
        std::cout << "Выбор: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');  
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }
        std::cin.ignore(10000, '\n');  

        if (choice == 3) break;
        if (choice != 1 && choice != 2) {
            std::cout << "Неверный выбор!\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

std::cout << "\nВведите путь к файлу или папке: ";

std::string pathStr;
if (!std::getline(std::cin, pathStr)) {
    std::cout << "Ошибка чтения пути!\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    continue;
}

if (pathStr.empty()) {
    std::cout << "Путь не введён!\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    continue;
}

// === УМНАЯ ОБРАБОТКА ПУТИ ===
// 1. Убираем кавычки в начале и конце (когда пользователь копирует из проводника или перетаскивает файл)
if (pathStr.size() >= 2 && pathStr.front() == '"' && pathStr.back() == '"') {
    pathStr = pathStr.substr(1, pathStr.size() - 2);
}

// 2. Убираем пробелы, табы, переводы строк с конца (часто остаются при копировании)
pathStr.erase(std::find_if(pathStr.rbegin(), pathStr.rend(), 
            [](unsigned char ch) { return !std::isspace(ch); }).base(), pathStr.end());

// 3. Если после очистки пусто — ошибка
if (pathStr.empty()) {
    std::cout << "Путь пустой после обработки!\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    continue;
}

// 4. Преобразуем все возможные слеши в нормальные (на всякий случай)
std::replace(pathStr.begin(), pathStr.end(), '\\', '/');

// === Теперь создаём fs::path ===
fs::path root(pathStr);

if (!fs::exists(root)) {
    std::cout << "Указанный путь не существует или недоступен:\n";
    std::cout << "→ " << root << "\n\n";
    std::cout << "(Проверьте правильность пути и права доступа)\n";
    std::this_thread::sleep_for(std::chrono::seconds(4));
    continue;
}

        std::vector<fs::path> files;
        if (choice == 1) {
            files.push_back(root);
        } else {
            try {
                for (const auto& entry : fs::recursive_directory_iterator(root)) {
                    if (entry.is_regular_file()) {
                        files.push_back(entry.path());
                    }
                }
            } catch (...) {
                std::cout << "Ошибка при чтении папки (возможно, нет доступа).\n";
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }
        }

        if (files.empty()) {
            std::cout << "Файлы не найдены!\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        std::vector<FileMetadata> results;
        std::cout << "\nАнализ " << files.size() << " файл(ов)...\n\n";

        for (size_t i = 0; i < files.size(); ++i) {
            const auto& file = files[i];
            std::cout << "[" << (i + 1) << "/" << files.size() << "] " << file.filename().string() << " → ";

            bool analyzed = false;
            for (const auto& analyzer : analyzers) {
                if (analyzer->canAnalyze(file)) {
                    try {
                        auto meta = analyzer->analyze(file);
                        results.push_back(std::move(meta));
                        std::cout << "УСПЕШНО\n";
                    } catch (const std::exception& e) {
                        std::cout << "ОШИБКА: " << e.what() << "\n";
                    } catch (...) {
                        std::cout << "НЕИЗВЕСТНАЯ ОШИБКА\n";
                    }
                    analyzed = true;
                    break;
                }
            }
            if (!analyzed) {
                std::cout << "формат не поддерживается\n";
            }
        }

        if (results.empty()) {
            std::cout << "\nНичего не проанализировано.\n";
        } else {
            std::cout << "\nГОТОВО! Обработано файлов: " << results.size() << "\n\n";
            std::cout << "1 — Сохранить в JSON\n";
            std::cout << "2 — Сохранить в TXT\n";
            std::cout << "3 — Показать в консоли\n";
            std::cout << "Выбор: ";

            int save;
            if (!(std::cin >> save)) save = 3;
            std::cin.ignore(10000, '\n');

            if (save == 1) {
                nlohmann::json j = nlohmann::json::array();
                for (const auto& m : results) j.push_back(m.toJson());
                std::ofstream f("результат_анализа.json");
                if (f.is_open()) {
                    f << j.dump(4);
                    std::cout << "Сохранено в результат_анализа.json\n";
                }
            }
            else if (save == 2) {
                std::ofstream f("результат_анализа.txt");
                if (f.is_open()) {
                    for (const auto& m : results) {
                        f << m.toString() << "\n";
                    }
                    std::cout << "Сохранено в результат_анализа.txt\n";
                }
            }
            else {
                std::cout << "\n";
                for (const auto& m : results) {
                    std::cout << m.toString();
                }
            }
        }

        std::cout << "\nНажмите Enter для продолжения...";
        std::cin.get();
    }

    std::cout << "Программа завершена. До свидания!\n";
    return 0;
}