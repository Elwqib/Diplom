cmake_minimum_required(VERSION 3.21)
project(DIPLOM_CMAKE LANGUAGES CXX)

cmake_policy(SET CMP0167 OLD)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# Указываем, где искать библиотеки
set(CMAKE_PREFIX_PATH "C:/vcpkg/vcpkg/installed/x64-windows")

# add_executable ДО target_*
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/analyzers/BaseAnalyzer.cpp
    src/analyzers/AudioAnalyzer.cpp
    src/analyzers/JpegAnalyzer.cpp
    src/analyzers/PdfAnalyzer.cpp
    src/analyzers/DocxAnalyzer.cpp
)

# find_package ПОСЛЕ toolchain
find_package(Boost REQUIRED COMPONENTS filesystem program_options)
find_package(taglib CONFIG REQUIRED)
find_package(tinyxml2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# target_* ПОСЛЕ add_executable
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    "C:/vcpkg/vcpkg/installed/x64-windows/include"
)

target_link_libraries(${PROJECT_NAME}
    Boost::filesystem
    Boost::program_options
    TagLib::TagLib
    tinyxml2::tinyxml2
    nlohmann_json::nlohmann_json
)