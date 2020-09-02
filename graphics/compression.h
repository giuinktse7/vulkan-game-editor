#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "lzma.h"

struct LZMA
{
  static std::vector<uint8_t> decompressFile(const std::filesystem::path &filepath);
  static std::vector<uint8_t> decompress(const std::vector<uint8_t> &buffer);

  // Level is between 0 (no compression), 9 (slow compression, small output).
  std::string compress(const std::string &in, int level);

private:
  static std::vector<uint8_t> decompressRaw(const std::vector<uint8_t> &in, lzma_options_lzma &options);
};