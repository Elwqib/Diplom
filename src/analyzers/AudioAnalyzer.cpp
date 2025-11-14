// AudioAnalyzer.cpp
#include "AudioAnalyzer.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <algorithm>
#include <windows.h>        
#include <string>

bool AudioAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".mp3" || ext == ".flac" || ext == ".wav" || ext == ".ogg" ||
           ext == ".m4a" || ext == ".aac" || ext == ".wma";
}

FileMetadata AudioAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

   
    std::wstring wpath = path.wstring();
    int size_needed = WideCharToMultiByte(CP_ACP, 0, wpath.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size_needed <= 0) {
        meta.setError("Ошибка конвертации пути для TagLib");
        return meta;
    }

    std::string ansi_path(size_needed, 0);
    WideCharToMultiByte(CP_ACP, 0, wpath.c_str(), -1, &ansi_path[0], size_needed, nullptr, nullptr);
    ansi_path.pop_back(); 

    TagLib::FileRef f(ansi_path.c_str());

    if (f.isNull() || !f.tag()) {
        meta.setError("Не удалось прочитать теги (файл повреждён или не поддерживается)");
        return meta;
    }

    TagLib::Tag* tag = f.tag();
    if (!tag->artist().isEmpty())
        meta.set("Исполнитель", tag->artist().toCString(true));
    if (!tag->album().isEmpty())
        meta.set("Альбом", tag->album().toCString(true));
    if (!tag->title().isEmpty())
        meta.set("Название", tag->title().toCString(true));
    if (tag->year() > 0)
        meta.set("Год", static_cast<int64_t>(tag->year()));
    if (tag->track() > 0)
        meta.set("Трек №", static_cast<int64_t>(tag->track()));
    if (!tag->genre().isEmpty())
        meta.set("Жанр", tag->genre().toCString(true));

    if (f.audioProperties()) {
        auto* p = f.audioProperties();
        meta.set("Длительность (сек)", static_cast<int64_t>(p->lengthInSeconds()));
        meta.set("Битрейт (kbps)", static_cast<int64_t>(p->bitrate()));
        meta.set("Частота (Hz)", static_cast<int64_t>(p->sampleRate()));
        meta.set("Каналы", static_cast<int64_t>(p->channels()));
    }

    return meta;
}