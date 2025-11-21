#include "JpegAnalyzer.h"
#include <exiv2/exiv2.hpp>
#include <algorithm>

bool JpegAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".jpg" || ext == ".jpeg";
}

FileMetadata JpegAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    try {
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(path.u8string());
        if (!image) {
            meta.setError("Не удалось открыть изображение");
            return meta;
        }

        image->readMetadata();
        const Exiv2::ExifData& exif = image->exifData();

        if (exif.empty()) {
            meta.set("EXIF", "Отсутствует");
            return meta;
        }

        auto get = [&](const std::string& key) -> std::string {
            auto it = exif.findKey(Exiv2::ExifKey(key));
            if (it == exif.end()) return "";
            return it->toString();
        };

        auto setIf = [&](const std::string& title, const std::string& key) {
            std::string val = get(key);
            if (!val.empty()) meta.set(title, val);
        };

        setIf("Дата съёмки", "Exif.Photo.DateTimeOriginal");
        setIf("Камера", "Exif.Image.Make");
        setIf("Модель камеры", "Exif.Image.Model");
        setIf("ISO", "Exif.Photo.ISOSpeedRatings");
        setIf("Выдержка", "Exif.Photo.ExposureTime");
        setIf("Диафрагма", "Exif.Photo.FNumber");
        setIf("Фокусное расстояние", "Exif.Photo.FocalLength");

        // GPS
        std::string lat = get("Exif.GPSInfo.GPSLatitude");
        std::string lon = get("Exif.GPSInfo.GPSLongitude");
        std::string latRef = get("Exif.GPSInfo.GPSLatitudeRef");
        std::string lonRef = get("Exif.GPSInfo.GPSLongitudeRef");
        if (!lat.empty() && !lon.empty()) {
            meta.set("Геолокация", latRef + " " + lat + ", " + lonRef + " " + lon);
        }

    } catch (...) {
        meta.setError("Ошибка чтения EXIF");
    }

    return meta;
}