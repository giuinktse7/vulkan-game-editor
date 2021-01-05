#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace File
{
    std::vector<uint8_t> read(const char *filename);
    std::vector<uint8_t> read(const std::string &filename);
    std::vector<uint8_t> read(const std::filesystem::path &path);

    void write(const std::filesystem::path &filepath, std::vector<uint8_t> &&buffer);

} // namespace File