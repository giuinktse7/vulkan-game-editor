#pragma once

#include <filesystem>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

namespace File
{
    std::vector<uint8_t> read(const char *filename);
    std::vector<uint8_t> read(const std::string &filename);
    std::vector<uint8_t> read(const std::filesystem::path &path);

    void write(const std::filesystem::path &filepath, std::vector<uint8_t> &&buffer);
    void writeJson(const std::filesystem::path &filepath, nlohmann::json &&json);

    bool exists(const std::filesystem::path &path);
    bool createDirectory(const std::filesystem::path &path);
    bool createDirectories(const std::filesystem::path &path);

} // namespace File