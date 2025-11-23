cmake_minimum_required(VERSION 3.17)
project(MetadataAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# путь к vcpkg (проверь, что он реальный!)
set(VCPKG_ROOT "C:/vcpkg/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")

# поиск библиотек в vcpkg
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x64-windows")

find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(taglib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/analyzers/BaseAnalyzer.cpp
    src/analyzers/JpegAnalyzer.cpp
    src/analyzers/AudioAnalyzer.cpp
    src/analyzers/PdfAnalyzer.cpp
    src/analyzers/DocxAnalyzer.cpp
    src/analyzers/ZipAnalyzer.cpp
    src/analyzers/PsdAnalyzer.cpp 
    src/analyzers/Mp4Analyzer.cpp 
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/analyzers
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::filesystem
    TagLib::TagLib
    nlohmann_json::nlohmann_json
    exiv2lib            
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

if(WIN32)
    set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/x64-windows")
    set(DLL_SRC_DEBUG "${VCPKG_INSTALLED_DIR}/debug/bin")
    set(DLL_SRC_RELEASE "${VCPKG_INSTALLED_DIR}/bin")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<$<CONFIG:Debug>:${DLL_SRC_DEBUG}>$<$<CONFIG:Release>:${DLL_SRC_RELEASE}>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Копирую DLL из vcpkg..."
        VERBATIM
    )
endif()
