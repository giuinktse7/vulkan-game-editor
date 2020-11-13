#pragma once

#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

#include "map.h"
#include "otbm.h"
#include "version.h"

class LoadMap
{
public:
  static std::pair<std::optional<Map>, std::optional<std::string>> loadMap(std::filesystem::path &path);

private:
  static bool isValidOTBMVersion(uint32_t value);
};

class LoadBuffer
{
public:
  LoadBuffer(std::vector<uint8_t> &&buffer);

  uint8_t nextU8();
  uint16_t nextU16();
  uint32_t nextU32();
  uint64_t nextU64();
  std::pair<uint8_t, OTBM::Node_t> readToken();
  void skip(size_t amount);

  std::string nextString(size_t size);
  std::string nextString();
  std::string nextLongString();

private:
  std::vector<uint8_t>::iterator cursor;
  std::vector<uint8_t> buffer;
};
