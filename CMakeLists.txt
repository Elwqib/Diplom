cmake_minimum_required(VERSION 3.17)
project(MetadataAnalyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(VCPKG_ROOT "C:/vcpkg/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain file")

# Только для поиска заголовков и CONFIG
list(APPEND CMAKE_PREFIX_PATH "${VCPKG_ROOT}/installed/x64-windows")


find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(taglib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(exiv2 CONFIG REQUIRED)
find_package(duckx CONFIG REQUIRED)


add_executable(${PROJECT_NAME}
    src/main.cpp
    src/analyzers/BaseAnalyzer.cpp
    src/analyzers/JpegAnalyzer.cpp
    src/analyzers/AudioAnalyzer.cpp
    src/analyzers/PdfAnalyzer.cpp
    src/analyzers/DocxAnalyzer.cpp
    
)

target_include_directories(${PROJECT_NAME} PRIVATE src src/analyzers)


target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::filesystem
    TagLib::TagLib
    nlohmann_json::nlohmann_json
    exiv2lib                  # 
)


if(MSVC)
    # Обязательно указываем, что исходный код в UTF-8, а не в CP1251
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)

    # Дополнительно (на случай очень старого VS) — принудительно ставим флаг
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/MTd>
        $<$<CONFIG:Release>:/MT>
    )
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