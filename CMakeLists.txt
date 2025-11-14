cmake_minimum_required(VERSION 3.17)
project(MetadataAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ──────────────────────── ТВОЙ НЕСТАНДАРТНЫЙ vcpkg ────────────────────────
set(VCPKG_ROOT "C:/vcpkg/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")

# Пути к заголовкам и библиотекам (обязательно оба!)
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x64-windows")

# Критически важно для твоего случая — вручную указываем, где лежат .lib
link_directories("${VCPKG_ROOT}/installed/x64-windows/lib")
link_directories("${VCPKG_ROOT}/installed/x64-windows/debug/lib")  # на всякий случай для Debug

# ──────────────────────── Статическая CRT (без vcruntime140.dll) ────────────────────────
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
set(VCPKG_TARGET_TRIPLET x64-windows)

# ──────────────────────── Пакеты ────────────────────────
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(taglib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)        # ← это находит exiv2lib автоматически
find_package(duckx CONFIG REQUIRED)

# ──────────────────────── Исполняемый файл ────────────────────────
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/analyzers/BaseAnalyzer.cpp
    src/analyzers/JpegAnalyzer.cpp
    src/analyzers/AudioAnalyzer.cpp
    src/analyzers/PdfAnalyzer.cpp
    src/analyzers/DocxAnalyzer.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src/analyzers)

# ──────────────────────── Линковка (главное — exiv2lib, а не exiv2) ────────────────────────
target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::filesystem
    TagLib::TagLib
    nlohmann_json::nlohmann_json
    exiv2lib                  # ←←←←←←←←←←←←←←←←←←←←←←←←←←← именно так!
    duckx::duckx
)

# ──────────────────────── Компилятор ────────────────────────
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()