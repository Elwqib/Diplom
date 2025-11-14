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
        auto image = Exiv2::ImageFactory::open(path.string());
        image->readMetadata();
        Exiv2::ExifData& exif = image->exifData();

        meta.set("Ширина", image->pixelWidth());
        meta.set("Высота", image->pixelHeight());

        if (auto it = exif.findKey(Exiv2::ExifKey("Exif.Photo.DateTimeOriginal")); it != exif.end())
            meta.set("Дата съёмки", it->toString());

        if (auto make = exif.findKey(Exiv2::ExifKey("Exif.Image.Make")); make != exif.end())
            if (auto model = exif.findKey(Exiv2::ExifKey("Exif.Image.Model")); model != exif.end())
                meta.set("Камера", make->toString() + " " + model->toString());

        // GPS
        try {
            std::string lat = exif["Exif.GPSInfo.GPSLatitude"].toString();
            std::string lon = exif["Exif.GPSInfo.GPSLongitude"].toString();
            if (!lat.empty() && !lon.empty()) {
                std::string latRef = exif["Exif.GPSInfo.GPSLatitudeRef"].toString();
                std::string lonRef = exif["Exif.GPSInfo.GPSLongitudeRef"].toString();
                meta.set("Геолокация", latRef + " " + lat + ", " + lonRef + " " + lon);
            }
        } catch (...) {}

    } catch (const std::exception& e) {
        meta.setError(std::string("Ошибка EXIF: ") + e.what());
    } catch (...) {
        meta.setError("Неизвестная ошибка при чтении EXIF");
    }

    return meta;
}