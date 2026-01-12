cmake_minimum_required(VERSION 3.17)
project(MetadataAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------
# vcpkg
# ------------------------------------------------------------
# Укажи путь к папке vcpkg (где лежит scripts/buildsystems/vcpkg.cmake)
# Пример: C:/vcpkg
# Можно переопределить при конфигурации:
#   cmake -DVCPKG_ROOT=C:/vcpkg -B build
if(NOT DEFINED VCPKG_ROOT)
    set(VCPKG_ROOT "C:/vcpkg/vcpkg")
endif()

set(CMAKE_TOOLCHAIN_FILE
    "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "vcpkg toolchain file"
)

# поиск библиотек в vcpkg (для x64-windows)
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x64-windows")

# ------------------------------------------------------------
# packages
# ------------------------------------------------------------
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(taglib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# ------------------------------------------------------------
# target
# ------------------------------------------------------------
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
    src/analyzers/PngAnalyzer.cpp
    src/utils/IntegrityChecker.cpp
    src/utils/VirusTotalChecker.cpp
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
    cpr::cpr
    OpenSSL::SSL
    OpenSSL::Crypto
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
endif()

# ------------------------------------------------------------
# Копирование DLL из vcpkg + vt_api_key.txt рядом с exe
# ------------------------------------------------------------
if(WIN32)
    set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/x64-windows")
    set(DLL_SRC_DEBUG   "${VCPKG_INSTALLED_DIR}/debug/bin")
    set(DLL_SRC_RELEASE "${VCPKG_INSTALLED_DIR}/bin")

    # Копируем нужные DLL в папку с exe (Debug/Release)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<IF:$<CONFIG:Debug>,${DLL_SRC_DEBUG},${DLL_SRC_RELEASE}>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMENT "Копирую DLL из vcpkg..."
        VERBATIM
    )

    # (необязательно) копируем vt_api_key.txt рядом с exe, если он есть в корне проекта
    set(VT_KEY_FILE "${CMAKE_SOURCE_DIR}/vt_api_key.txt")
    if(EXISTS "${VT_KEY_FILE}")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VT_KEY_FILE}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/vt_api_key.txt"
            COMMENT "Копирую vt_api_key.txt рядом с exe..."
            VERBATIM
        )
    endif()
endif()