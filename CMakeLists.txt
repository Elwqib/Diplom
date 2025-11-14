cmake_minimum_required(VERSION 3.17)
project(MetadataAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(VCPKG_ROOT "C:/vcpkg/vcpkg")   
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")


list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x64-windows")

#  Статическая CRT  
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
set(VCPKG_TARGET_TRIPLET x64-windows)


find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(taglib CONFIG REQUIRED)          #
find_package(nlohmann_json CONFIG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(duckx CONFIG REQUIRED)            

# Исполняемый файл 
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/analyzers/BaseAnalyzer.cpp
    src/analyzers/JpegAnalyzer.cpp
    src/analyzers/AudioAnalyzer.cpp
    src/analyzers/PdfAnalyzer.cpp
    src/analyzers/DocxAnalyzer.cpp
 
)

target_include_directories(${PROJECT_NAME} PRIVATE src/analyzers)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::filesystem
    TagLib::TagLib
    nlohmann_json::nlohmann_json
    exiv2
    duckx::duckx
)

# Настройки компилятора 
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()