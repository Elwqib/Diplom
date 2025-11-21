#include "AudioAnalyzer.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

bool AudioAnalyzer::canAnalyze(const fs::path& path) const {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".mp3" || ext == ".flac" || ext == ".wav" || ext == ".ogg" ||
           ext == ".m4a" || ext == ".aac" || ext == ".wma";
}

FileMetadata AudioAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta(path);
    addBasicInfo(meta, path);

    TagLib::FileRef f(path.wstring().c_str());
    if (f.isNull() || !f.tag()) {
        meta.setError("Не удалось прочитать теги");
        return meta;
    }

    TagLib::Tag* tag = f.tag();
    if (!tag->artist().isEmpty()) meta.set("Исполнитель", tag->artist().toCString(true));
    if (!tag->album().isEmpty()) meta.set("Альбом", tag->album().toCString(true));
    if (!tag->title().isEmpty()) meta.set("Название", tag->title().toCString(true));
    if (tag->year() > 0) meta.set("Год", static_cast<int64_t>(tag->year()));
    if (tag->track() > 0) meta.set("Трек №", static_cast<int64_t>(tag->track()));
    if (!tag->genre().isEmpty()) meta.set("Жанр", tag->genre().toCString(true));

    if (f.audioProperties()) {
        auto* p = f.audioProperties();
        meta.set("Длительность (сек)", static_cast<int64_t>(p->lengthInSeconds()));
        meta.set("Битрейт (kbps)", static_cast<int64_t>(p->bitrate()));
        meta.set("Частота (Hz)", static_cast<int64_t>(p->sampleRate()));
        meta.set("Каналы", static_cast<int64_t>(p->channels()));
    }

    return meta;
}