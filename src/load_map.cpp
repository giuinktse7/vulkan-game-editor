#include "load_map.h"

#include "debug.h"
#include "file.h"

std::pair<std::optional<Map>, std::optional<std::string>> LoadMap::loadMap(std::filesystem::path &path)
{
#define RETURN_MAP_LOAD_ERROR(_message_) \
  do                                     \
  {                                      \
    std::ostringstream s;                \
    s << _message_;                      \
    return {std::nullopt, s.str()};      \
  } while (false)

  if (!std::filesystem::exists(path) || std::filesystem::is_directory(path))
  {
    RETURN_MAP_LOAD_ERROR("Could not find map at path: " << path.string());
  }

  LoadBuffer buffer(File::read(path));

  std::string header = buffer.nextString(4);
  if (header != "OTBM")
  {
    RETURN_MAP_LOAD_ERROR("Bad format: The first four bytes of the .otbm file must be \"OTBM\", but they were " << header);
  }

  {
    auto [start, nodeType] = buffer.readToken();
    if (start != OTBM::Token::Start || nodeType != OTBM::Node_t::Root)
    {
      RETURN_MAP_LOAD_ERROR("Invalid OTBM file. Expected OTBM::Token::Start (0xFE) followed by OTBM::Node_t::Root (0x0).");
    }
  }

  uint32_t otbmVersion = buffer.nextU32();
  if (!isValidOTBMVersion(otbmVersion))
  {
    RETURN_MAP_LOAD_ERROR("Unsupported OTBM version: " << otbmVersion << ". Supported versions (name, value): (OTBM1, 0), (OTBM2, 1), (OTBM3, 2), (OTBM4, 3).");
  }

  return {std::nullopt, std::nullopt};
}

//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>>LoadBuffer>>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>

LoadBuffer::LoadBuffer(std::vector<uint8_t> &&buffer)
    : cursor(buffer.begin()), buffer(std::move(buffer))
{
}

uint8_t LoadBuffer::nextU8()
{
  if (*cursor == static_cast<uint8_t>(OTBM::Token::Escape))
  {
    ++cursor;
  }

  uint8_t result = *cursor;
  ++cursor;
  return result;
}

uint16_t LoadBuffer::nextU16()
{
  uint16_t result = 0;
  int shift = 0;
  while (shift < 2)
  {
    if (*cursor == static_cast<uint8_t>(OTBM::Token::Escape))
    {
      ++cursor;
    }

    result += (*cursor) << (8 * shift);
    ++cursor;
    ++shift;
  }

  return result;
}

uint32_t LoadBuffer::nextU32()
{
  uint32_t result = 0;
  int shift = 0;
  while (shift < 4)
  {
    if (*cursor == static_cast<uint8_t>(OTBM::Token::Escape))
    {
      ++cursor;
    }

    result += (*cursor) << (8 * shift);
    ++cursor;
    ++shift;
  }

  return result;
}

uint64_t LoadBuffer::nextU64()
{
  uint32_t result = 0;
  int shift = 0;
  while (shift < 8)
  {
    if (*cursor == static_cast<uint8_t>(OTBM::Token::Escape))
    {
      ++cursor;
    }

    result += (*cursor) << (8 * shift);
    ++cursor;
    ++shift;
  }

  return result;
}

std::string LoadBuffer::nextString(size_t size)
{
  std::string value(reinterpret_cast<char *>(&(*cursor)), size);

  cursor += size;
  return value;
}

std::string LoadBuffer::nextString()
{
  uint16_t size = nextU16();
  std::string value(reinterpret_cast<char *>(&(*cursor)), static_cast<size_t>(size));

  cursor += size;
  return value;
}

std::string LoadBuffer::nextLongString()
{
  uint16_t size = nextU32();
  std::string value(reinterpret_cast<char *>(&(*cursor)), static_cast<size_t>(size));

  cursor += size;
  return value;
}

std::pair<uint8_t, OTBM::Node_t> LoadBuffer::readToken()
{
  return {nextU8(), static_cast<OTBM::Node_t>(nextU8())};
}

void LoadBuffer::skip(size_t amount)
{
  while (amount > 0)
  {
    if (*cursor == static_cast<uint8_t>(OTBM::Token::Escape))
    {
      ++cursor;
    }

    ++cursor;
    --amount;
  }
}

bool LoadMap::isValidOTBMVersion(uint32_t value)
{
  switch (static_cast<OTBMVersion>(value))
  {
  case OTBMVersion::OTBM1:
  case OTBMVersion::OTBM2:
  case OTBMVersion::OTBM3:
  case OTBMVersion::OTBM4:
    return true;
  default:
    return false;
  }
}
