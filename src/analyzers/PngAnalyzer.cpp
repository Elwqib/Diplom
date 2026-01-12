#include "PngAnalyzer.h"

#include <exiv2/exiv2.hpp>
#include <algorithm>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>   // memcpy

#ifdef _WIN32
  #include <intrin.h> // _byteswap_ulong
#endif

static uint32_t bswap32(uint32_t v) {
#ifdef _WIN32
    return _byteswap_ulong(v);
#else
    return ((v & 0x000000FFu) << 24) |
           ((v & 0x0000FF00u) << 8)  |
           ((v & 0x00FF0000u) >> 8)  |
           ((v & 0xFF000000u) >> 24);
#endif
}

bool PngAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext == ".png";
}

FileMetadata PngAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    if (!fs::exists(path)) {
        meta.setError("Файл не существует или недоступен: " + path.u8string());
        return meta;
    }

    try {
        // 1) Читаем файл целиком в память (так решаем проблему с кириллицей в пути для Exiv2)
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            meta.setError("Не удалось открыть файл: " + path.u8string());
            return meta;
        }

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());

        if (data.size() < 33) { // сигнатура (8) + length(4)+type(4)+width(4)+height(4)+прочее
            meta.setError("Файл слишком мал для PNG");
            return meta;
        }

        // 2) Проверка сигнатуры PNG (8 байт)
        const std::vector<uint8_t> pngSig{0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
        if (!std::equal(pngSig.begin(), pngSig.end(), data.begin())) {
            meta.setError("Не является PNG-файлом (неверная сигнатура)");
            return meta;
        }

        meta.set("Формат", "PNG image");

        // 3) Проверяем, что первый chunk — IHDR
        char type[5] = {0};
        type[0] = static_cast<char>(data[12]);
        type[1] = static_cast<char>(data[13]);
        type[2] = static_cast<char>(data[14]);
        type[3] = static_cast<char>(data[15]);

        if (std::string(type, 4) != "IHDR") {
            meta.setError("Некорректный PNG: не найден IHDR");
            return meta;
        }

        // 4) Ширина/высота (big-endian) находятся после type
        uint32_t width_be = 0, height_be = 0;
        std::memcpy(&width_be,  &data[16], 4);
        std::memcpy(&height_be, &data[20], 4);

        const uint32_t width  = bswap32(width_be);
        const uint32_t height = bswap32(height_be);

        meta.set("Ширина (px)", static_cast<int64_t>(width));
        meta.set("Высота (px)", static_cast<int64_t>(height));

        // 5) Exiv2 из памяти:
        // ТВОЯ версия Exiv2 ожидает open(BasicIo::UniquePtr), поэтому делаем UniquePtr на MemIo.
        Exiv2::BasicIo::UniquePtr io(new Exiv2::MemIo(
            reinterpret_cast<Exiv2::byte*>(data.data()),
            static_cast<long>(data.size())
        ));

        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(std::move(io));
        if (!image) {
            meta.set("Метаданные", "Отсутствуют или нечитаемы");
            return meta;
        }

        image->readMetadata();

        const Exiv2::XmpData& xmp  = image->xmpData();
        const Exiv2::ExifData& exif = image->exifData();

        auto getXmp = [&](const std::string& key) -> std::string {
            auto it = xmp.findKey(Exiv2::XmpKey(key));
            return (it != xmp.end()) ? it->toString() : "";
        };

        auto getExif = [&](const std::string& key) -> std::string {
            auto it = exif.findKey(Exiv2::ExifKey(key));
            return (it != exif.end()) ? it->toString() : "";
        };

        auto setIf = [&](const std::string& title, const std::string& val) {
            if (!val.empty()) meta.set(title, val);
        };

        // Часто встречающиеся XMP-поля
        setIf("Дата (XMP)",            getXmp("Xmp.xmp.CreateDate"));
        setIf("Дата изменения (XMP)",  getXmp("Xmp.xmp.ModifyDate"));
        setIf("Автор (XMP)",           getXmp("Xmp.dc.creator"));
        setIf("Описание (XMP)",        getXmp("Xmp.dc.description"));
        setIf("Ключевые слова (XMP)",  getXmp("Xmp.dc.subject"));
        setIf("Программа (XMP)",       getXmp("Xmp.xmp.CreatorTool"));

        // Иногда EXIF в PNG тоже встречается
        setIf("Дата оригинала (EXIF)", getExif("Exif.Photo.DateTimeOriginal"));
        setIf("Устройство (EXIF)",     getExif("Exif.Image.Model"));
        setIf("Производитель (EXIF)",  getExif("Exif.Image.Make"));

    } catch (const Exiv2::Error& e) {
        meta.setError("Ошибка Exiv2 при анализе PNG: " + std::string(e.what()));
    } catch (const std::exception& e) {
        meta.setError("Ошибка анализа PNG: " + std::string(e.what()));
    } catch (...) {
        meta.setError("Неизвестная ошибка анализа PNG");
    }

    return meta;
}
