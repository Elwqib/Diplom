#include "JpegAnalyzer.h"

#include <exiv2/exiv2.hpp>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>
#include <exception>

bool JpegAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Ограничимся нормальными JPEG
    return ext == ".jpg" || ext == ".jpeg";
}

FileMetadata JpegAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    try {
        // path.string() — чтобы не упасть на путях в Windows
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(path.string());
        if (!image) {
            meta.set("EXIF", "Не удалось открыть изображение для чтения метаданных");
            return meta;
        }

        image->readMetadata();
        const Exiv2::ExifData& exif = image->exifData();
        const Exiv2::IptcData& iptc = image->iptcData();
        const Exiv2::XmpData&  xmp  = image->xmpData();

        // Вообще нет метаданных
        if (exif.begin() == exif.end() &&
            iptc.begin() == iptc.end() &&
            xmp.begin()  == xmp.end())
        {
            meta.set("Метаданные", "Отсутствуют (файл не содержит EXIF/IPTC/XMP)");
            return meta;
        }

        // Удобный доступ к EXIF по ключу
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

        // ==== БАЗОВЫЕ ПОЛЯ EXIF (по-русски) ====
        setIf("Дата съёмки",               "Exif.Photo.DateTimeOriginal");
        setIf("Дата оцифровки",            "Exif.Photo.DateTimeDigitized");
        setIf("Камера (производитель)",    "Exif.Image.Make");
        setIf("Модель камеры",             "Exif.Image.Model");
        setIf("ISO",                       "Exif.Photo.ISOSpeedRatings");
        setIf("Выдержка",                  "Exif.Photo.ExposureTime");
        setIf("Диафрагма (F-число)",       "Exif.Photo.FNumber");
        setIf("Программа экспозиции",      "Exif.Photo.ExposureProgram");
        setIf("Режим замера экспозиции",   "Exif.Photo.MeteringMode");
        setIf("Тип сцены",                 "Exif.Photo.SceneCaptureType");
        setIf("Фокусное расстояние",       "Exif.Photo.FocalLength");
        setIf("Программное обеспечение",   "Exif.Image.Software");
        setIf("Ширина изображения (px)",   "Exif.Photo.PixelXDimension");
        setIf("Высота изображения (px)",   "Exif.Photo.PixelYDimension");

        // ==== GPS: явная метка + сами координаты ====
        std::string lat    = get("Exif.GPSInfo.GPSLatitude");
        std::string lon    = get("Exif.GPSInfo.GPSLongitude");
        std::string latRef = get("Exif.GPSInfo.GPSLatitudeRef");
        std::string lonRef = get("Exif.GPSInfo.GPSLongitudeRef");

        if (!lat.empty() && !lon.empty()) {
            meta.set("Содержит GPS-координаты", true);
            meta.set("GPS (как в EXIF)",
                     latRef + " " + lat + ", " + lonRef + " " + lon);
        } else {
            meta.set("Содержит GPS-координаты", false);
        }

        // ==== ОЦЕНКА ОБЪЁМА МЕТАДАННЫХ (без вывода сырого дампа) ====
        std::ostringstream exifDump, iptcDump, xmpDump;
        for (Exiv2::ExifData::const_iterator it = exif.begin(); it != exif.end(); ++it)
            exifDump << it->key() << " = " << it->value() << "\n";
        for (Exiv2::IptcData::const_iterator it = iptc.begin(); it != iptc.end(); ++it)
            iptcDump << it->key() << " = " << it->value() << "\n";
        for (Exiv2::XmpData::const_iterator it = xmp.begin(); it != xmp.end(); ++it)
            xmpDump << it->key() << " = " << it->value() << "\n";

        std::string exifStr = exifDump.str();
        std::string iptcStr = iptcDump.str();
        std::string xmpStr  = xmpDump.str();

        std::size_t metaBytes = exifStr.size() + iptcStr.size() + xmpStr.size();
        if (metaBytes > 64 * 1024) {
            meta.set("Подозрение: аномально много метаданных (возможное скрытие данных)", true);
            meta.set("Оценочный объём всех метаданных (байт)",
                     static_cast<int64_t>(metaBytes));
        }

        // ==== ПРОВЕРКА ДАННЫХ ПОСЛЕ КОНЦА JPEG (возможная стеганография) ====
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

    } catch (const std::exception& e) {
        meta.set("EXIF", std::string("Ошибка при чтении метаданных: ") + e.what());
    } catch (...) {
        meta.set("EXIF", "Неизвестная ошибка при чтении EXIF/IPTC/XMP");
    }

    return meta;
}
