#include "AudioAnalyzer.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>

bool AudioAnalyzer::canAnalyze(const fs::path& path) const {
    auto ext = path.extension().string();
    return ext == ".mp3" || ext == ".flac";
}

FileMetadata AudioAnalyzer::analyze(const fs::path& path) {
    FileMetadata meta;
    addBasicInfo(meta, path);

    TagLib::FileRef f(path.string().c_str());
    if (!f.isNull() && f.tag()) {
        TagLib::Tag* tag = f.tag();
        json tags;
        tags["Title"] = tag->title().toCString();
        tags["Artist"] = tag->artist().toCString();
        tags["Album"] = tag->album().toCString();
        tags["Year"] = tag->year();
        tags["Track"] = tag->track();
        meta.data["Tags"] = tags;
    } else {
        meta.data["Tags"] = "No tags";
    }
    return meta;
}