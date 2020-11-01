#include "file.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "debug.h"

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
