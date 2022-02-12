#include "file.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "debug.h"

std::vector<uint8_t> File::read(const char *filename)
{
    return File::read(std::string(filename));
}

std::vector<uint8_t> File::read(const std::filesystem::path &path)
{
    return File::read(path.string());
}

std::vector<uint8_t> File::read(const std::string &filepath)
{
    std::ifstream ifs(filepath, std::ios::binary | std::ios::ate);

    if (!ifs)
    {
        std::cout << std::filesystem::current_path() << std::endl;
        ABORT_PROGRAM("Could not find file: " + filepath);
    }

    auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto size = std::size_t(end - ifs.tellg());

    if (size == 0) // avoid undefined behavior
        return {};

    std::vector<uint8_t> buffer(size);

    if (!ifs.read((char *)buffer.data(), size))
        throw std::runtime_error("Could not read file: " + filepath);

    return buffer;
}

void File::write(const std::filesystem::path &filepath, std::vector<uint8_t> &&buffer)
{
    std::ofstream outfile(filepath, std::ios::out | std::ios::binary);
    outfile.write((const char *)buffer.data(), buffer.size());
}

bool File::exists(const std::filesystem::path &path)
{
    return std::filesystem::exists(path);
}

bool File::createDirectory(const std::filesystem::path &path)
{
    return std::filesystem::create_directory(path);
}

bool File::createDirectories(const std::filesystem::path &path)
{
    return std::filesystem::create_directories(path);
}
