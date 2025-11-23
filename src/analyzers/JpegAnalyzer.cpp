#include "JpegAnalyzer.h"
#include <exiv2/exiv2.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

bool JpegAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    // JPEG + PNG + TIFF (расширяем поддержку)
    return ext == ".jpg" || ext == ".jpeg" ||
           ext == ".png" || ext == ".tif"  || ext == ".tiff";
}

FileMetadata JpegAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    try {
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(path.u8string());
        if (!image) {
            meta.setError("Не удалось открыть изображение");
            return meta;
        }

        image->readMetadata();
        const Exiv2::ExifData& exif = image->exifData();
        const Exiv2::IptcData& iptc = image->iptcData();
        const Exiv2::XmpData&  xmp  = image->xmpData();

        if (exif.begin() == exif.end() &&
            iptc.begin() == iptc.end() &&
            xmp.begin()  == xmp.end())
        {
            meta.set("Метаданные", "Отсутствуют");
            return meta;
        }

        auto get = [&](const std::string& key) -> std::string {
            Exiv2::ExifKey k(key);
            Exiv2::ExifData::const_iterator it = exif.findKey(k);
            if (it == exif.end()) return "";
            return it->toString();
        };

        auto setIf = [&](const std::string& title, const std::string& key) {
            std::string val = get(key);
            if (!val.empty()) meta.set(title, val);
        };

        // Базовые поля EXIF
        setIf("Дата съёмки",        "Exif.Photo.DateTimeOriginal");
        setIf("Камера",             "Exif.Image.Make");
        setIf("Модель камеры",      "Exif.Image.Model");
        setIf("ISO",                "Exif.Photo.ISOSpeedRatings");
        setIf("Выдержка",           "Exif.Photo.ExposureTime");
        setIf("Диафрагма",          "Exif.Photo.FNumber");
        setIf("Фокусное расстояние","Exif.Photo.FocalLength");

        // GPS
        std::string lat    = get("Exif.GPSInfo.GPSLatitude");
        std::string lon    = get("Exif.GPSInfo.GPSLongitude");
        std::string latRef = get("Exif.GPSInfo.GPSLatitudeRef");
        std::string lonRef = get("Exif.GPSInfo.GPSLongitudeRef");
        if (!lat.empty() && !lon.empty()) {
            meta.set("Геолокация", latRef + " " + lat + ", " + lonRef + " " + lon);
        }

        // Сырые дампы EXIF/IPTC/XMP (обрезаем по длине)
        std::ostringstream exifDump, iptcDump, xmpDump;
        for (Exiv2::ExifData::const_iterator it = exif.begin(); it != exif.end(); ++it)
            exifDump << it->key() << " = " << it->value() << "\n";
        for (Exiv2::IptcData::const_iterator it = iptc.begin(); it != iptc.end(); ++it)
            iptcDump << it->key() << " = " << it->value() << "\n";
        for (Exiv2::XmpData::const_iterator it = xmp.begin(); it != xmp.end(); ++it)
            xmpDump << it->key() << " = " << it->value() << "\n";

        auto limitStr = [](const std::string& s) {
            const std::size_t MAX_LEN = 4000;
            if (s.size() <= MAX_LEN) return s;
            return s.substr(0, MAX_LEN) + "... (обрезано)";
        };

        std::string exifStr = exifDump.str();
        std::string iptcStr = iptcDump.str();
        std::string xmpStr  = xmpDump.str();

        if (!exifStr.empty()) meta.set("EXIF (raw)", limitStr(exifStr));
        if (!iptcStr.empty()) meta.set("IPTC (raw)", limitStr(iptcStr));
        if (!xmpStr.empty())  meta.set("XMP (raw)",  limitStr(xmpStr));

        std::size_t metaBytes = exifStr.size() + iptcStr.size() + xmpStr.size();
        if (metaBytes > 64 * 1024) {
            meta.set("Подозрение: аномально много метаданных (возможное скрытие данных)", true);
        }

        // Простая проверка на данные после конца JPEG (признак стего)
        if (ext == ".jpg" || ext == ".jpeg") {
            std::ifstream in(path, std::ios::binary | std::ios::ate);
            if (in) {
                std::streamsize size = in.tellg();
                if (size > 0) {
                    std::vector<unsigned char> buf(static_cast<std::size_t>(size));
                    in.seekg(0, std::ios::beg);
                    in.read(reinterpret_cast<char*>(buf.data()), size);

                    std::streamsize eoiPos = -1;
                    for (std::streamsize i = 0; i < size - 1; ++i) {
                        if (buf[static_cast<std::size_t>(i)]     == 0xFF &&
                            buf[static_cast<std::size_t>(i + 1)] == 0xD9)
                        {
                            eoiPos = i + 2;
                        }
                    }

                    if (eoiPos > 0 && size - eoiPos > 4096) {
                        meta.set("Подозрение: данные после EOI (возможная стеганография)", true);
                        meta.set("Размер данных после EOI (байт)",
                                 static_cast<int64_t>(size - eoiPos));
                    }
                }
            }
        }

    } catch (...) {
        meta.setError("Ошибка чтения метаданных (EXIF/IPTC/XMP)");
    }

    return meta;
}
