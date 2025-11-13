/*std::string Hash::sha256(const fs::path& path) {
    std::ifstream file(path.string(), std::ios::binary);
    if (!file) return "ERROR";

    std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
    return "sha256:" + std::to_string(data.size());
}/**/