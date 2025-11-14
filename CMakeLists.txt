cmake_minimum_required(VERSION 3.21)

# ОБЯЗАТЕЛЬНО ДО project()
set(CMAKE_TOOLCHAIN_FILE 
    "C:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake" 
    CACHE STRING "vcpkg toolchain file"
)

project(DIPLOM_CMAKE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Поиск пакетов
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(taglib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(duckx CONFIG REQUIRED)

# Исполняемый файл
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/analyzers/BaseAnalyzer.cpp
    src/analyzers/AudioAnalyzer.cpp
    src/analyzers/JpegAnalyzer.cpp
    src/analyzers/PdfAnalyzer.cpp
    src/analyzers/DocxAnalyzer.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::filesystem
    TagLib::TagLib
    nlohmann_json::nlohmann_json
    exiv2lib  
    duckx::duckx
)

# UTF-8
target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)