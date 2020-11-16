#include "load_map.h"

#include "debug.h"
#include "file.h"

namespace
{
  // The first four bytes of an .otbm file must be 'OTBM' or all 0 (\0\0\0\0).
  std::array<std::string, 2> OTBMFileIdentifiers{
      "OTBM",
      std::string(4, static_cast<char>(0))};

  std::string OtbmWildcard(4, static_cast<char>(0));

  bool validOtbmIdentifier(const std::string &identifier)
  {
    auto found = std::find(
        OTBMFileIdentifiers.begin(),
        OTBMFileIdentifiers.end(),
        identifier);

    return found != OTBMFileIdentifiers.end();
  }

  template <typename T>
  std::string operator+(const std::string &lhs, const T &rhs)
  {
    std::ostringstream s;
    s << lhs << rhs;
    return s.str();
  }
} // namespace

std::variant<Map, std::string> LoadMap::loadMap(std::filesystem::path &path)
{
  if (!std::filesystem::exists(path) || std::filesystem::is_directory(path))
  {
    return error("Could not find map at path: " + path.string());
  }

  LoadBuffer buffer(File::read(path));

  std::string otbmIdentifier = buffer.nextString(4);
  if (!validOtbmIdentifier(otbmIdentifier))
  {
    return error("Bad format: The first four bytes of the .otbm file must be"
                 "'OTBM' or '0000', but they were '" +
                 otbmIdentifier + "'");
  }

  OTBM::Node_t rootNodeType = buffer.readNodeStart();
  if (rootNodeType != OTBM::Node_t::Root)
  {
    return error("Invalid OTBM file. rootNodeType must be " + to_underlying(OTBM::Node_t::Root));
  }

  uint32_t otbmVersion = buffer.nextU32();
  if (!isValidOTBMVersion(otbmVersion))
  {
    std::string supportedVersions = "(OTBM1, 0), (OTBM2, 1), (OTBM3, 2), (OTBM4, 3)";
    return error("Unsupported OTBM version: " + std::to_string(otbmVersion) + ". Supported versions (name, value): " + supportedVersions + ".");
  }

  auto deserializer = OTBM::Deserializer::create(static_cast<OTBMVersion>(otbmVersion), buffer);
  if (!deserializer)
  {
    return error("There is no supported deserializer for OTBM version " + std::to_string(otbmVersion) + ".");
  }

  uint16_t width = buffer.nextU16();
  uint16_t height = buffer.nextU16();

  Map map(width, height);

  const OTB::VersionInfo &itemsOtb = Items::items.otbVersionInfo();

  OTB::VersionInfo mapOtb;

  mapOtb.majorVersion = buffer.nextU32();
  if (mapOtb.majorVersion > itemsOtb.majorVersion)
  {
    return error("Unsupported OTB major version: " + mapOtb.majorVersion);
  }
  else if (mapOtb.majorVersion < itemsOtb.majorVersion)
  {
    return error("Map was saved with major OTB version " + std::to_string(mapOtb.majorVersion) + " but the loaded items.otb uses major OTB version " + itemsOtb.majorVersion);
  }
  uint32_t minorOtbVersion = buffer.nextU32();

  if (minorOtbVersion != itemsOtb.minorVersion)
  {
    return error("Minor OTB versions differ. The loaded items.otb has version: '" + itemsOtb.show() + "' but the map was saved using OTB version '" + mapOtb.show() + "'.");
  }

  {
    OTBM::Node_t mapDataNodeType = buffer.readNodeStart();
    if (mapDataNodeType != OTBM::Node_t::MapData)
    {
      return error("Invalid OTBM file. Expected OTBM::Token::Start (0xFE) followed by OTBM::Node_t::MapData (0x2).");
    }

    // Read map attributes until the first map node begins
    while (buffer.peek() != OTBM::Token::Start)
    {
      uint8_t token = buffer.nextU8();
      switch (static_cast<OTBM::NodeAttribute>(token))
      {
      case OTBM::NodeAttribute::Description:
        map.setDescription(buffer.nextString());
        break;
      case OTBM::NodeAttribute::ExternalSpawnFile:
        map.setSpawnFilepath(buffer.nextString());
        break;
      case OTBM::NodeAttribute::ExternalHouseFile:
        map.setHouseFilepath(buffer.nextString());
        break;
      default:
        logWarning("Unknown map header node: " + token);
      }
    }

    uint32_t loadedNodes = 0;

    while (buffer.peek() != OTBM::Token::End)
    {
      OTBM::Node_t nodeType = buffer.readNodeStart();

      switch (static_cast<OTBM::Node_t>(nodeType))
      {
      case OTBM::Node_t::TileArea:
      {
        auto errorString = deserializeTileArea(buffer, *deserializer.get(), map);
        if (errorString)
        {
          return error(errorString.value());
        }
        break;
      }
      case OTBM::Node_t::Towns:
      {
        auto errorString = deserializeTowns(buffer, *deserializer.get(), map);
        if (errorString)
        {
          return error(errorString.value());
        }
        break;
      }
      case OTBM::Node_t::Waypoints:
      {
        // TODO
        break;
      }
      default:
        return error("Unknown nodeType (after map attributes).");
      }
    }

    buffer.readEnd();
  } // MapNode

  // End RootNode
  bool end = buffer.readEnd();
  if (!end)
  {
    ABORT_PROGRAM("Expected end.");
  }
  return map;
}

std::optional<std::string> LoadMap::deserializeTileArea(LoadBuffer &buffer, OTBM::Deserializer &deserializer, Map &map)
{
  uint16_t baseX = buffer.nextU16();
  uint16_t baseY = buffer.nextU16();
  uint8_t baseZ = buffer.nextU8();
  while (buffer.peek() != OTBM::Token::End)
  {
    OTBM::Node_t nodeType = buffer.readNodeStart();
    switch (nodeType)
    {
    case OTBM::Node_t::Tile:
    case OTBM::Node_t::Housetile:
    {
      uint8_t offsetX = buffer.nextU8();
      uint8_t offsetY = buffer.nextU8();

      const Position position(baseX + offsetX, baseY + offsetY, baseZ);

      if (map.getTile(position))
      {
        logWarning("[deserializeTileArea] Duplicate tile at " + position);
        buffer.skipNode(true);
        break;
      }

      Tile &tile = map.getOrCreateTile(position);

      if (nodeType == OTBM::Node_t::Housetile)
      {
        uint32_t houseId = buffer.nextU32();
        // TODO Add house to map
      }

      while (buffer.peek() != OTBM::Token::Start && buffer.peek() != OTBM::Token::End)
      {
        switch (static_cast<OTBM::NodeAttribute>(buffer.nextU8()))
        {
        case OTBM::NodeAttribute::TileFlags:
          tile.setFlags(buffer.nextU32());
          break;
        case OTBM::NodeAttribute::Item:
          // Load ground (if it has no attributes)
          tile.addItem(deserializer.deserializeCompactItem());
          break;
        default:
          return "[deserializeTileArea] Unsupported NodeAttribute (when parsing OTBM::NodeAttribute for a Tile).";
        }
      }

      // Load items
      while (buffer.peek() != OTBM::Token::End)
      {
        OTBM::Node_t nodeType = buffer.readNodeStart();
        switch (nodeType)
        {
        case OTBM::Node_t::Item:
          tile.addItem(deserializer.deserializeItem());
          break;
        default:
          return "[deserializeTileArea] Unknown OTBM::Node_t: " + to_underlying(nodeType);
        }

        buffer.readEnd();
      } // End Item
    }
    break;
    default:
      return "[deserializeTileArea] Unknown NodeType.";
    }

    bool end = buffer.readEnd();
    if (!end)
    {
      ABORT_PROGRAM("Expected Tile node end.");
    }
  }

  bool end = buffer.readEnd();
  if (!end)
  {
    return "[deserializeTileArea] Expected end.";
  }

  return std::nullopt;
}

std::optional<std::string> LoadMap::deserializeTowns(LoadBuffer &buffer, OTBM::Deserializer &deserializer, Map &map)
{
  while (buffer.peek() != OTBM::Token::End)
  {
    OTBM::Node_t townNodeType = buffer.readNodeStart();
    if (townNodeType != OTBM::Node_t::Town)
    {
      return "Expected OTBM::Node_t::Town";
    }

    uint32_t townId = buffer.nextU32();
    if (map.getTown(townId))
    {
      logWarning("Duplicate town id: " + std::to_string(townId) + ". Skipping town.");
      buffer.skipNode();
      continue;
    }

    Town town(townId);

    std::string name = buffer.nextString();
    town.setName(name);

    Position templePos = buffer.readPosition();
    town.setTemplePosition(templePos);

    map.addTown(std::move(town));

    bool end = buffer.readEnd();
    if (!end)
    {
      ABORT_PROGRAM("Expected Town node end.");
    }
  }

  buffer.readEnd();

  return std::nullopt;
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

void LoadMap::logWarning(std::string message)
{
  VME_LOG("[LoadMap warning] " << message);
}

std::variant<Map, std::string> LoadMap::error(std::string message)
{
  return message;
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

uint8_t LoadBuffer::peek() const
{
  return *cursor;
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

OTBM::Node_t LoadBuffer::readNodeStart()
{
  uint8_t tokenType = *cursor;
  ++cursor;

  if (tokenType != OTBM::Token::Start)
  {
    ABORT_PROGRAM("Expected OTBM::Token::Start");
  }

  uint8_t nodeType = *cursor;
  ++cursor;

  if (!OTBM::isNodeType(nodeType))
  {
    ABORT_PROGRAM("Expected OTBM::Token::Start");
  }

  return static_cast<OTBM::Node_t>(nodeType);
}

bool LoadBuffer::readEnd()
{
  uint8_t end = *cursor;
  ++cursor;

  return end == OTBM::Token::End;
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

void LoadBuffer::skipNode(bool remainAtEnd)
{
  size_t depth = 1;
  uint8_t previous = *cursor;
  while (depth > 0)
  {
    if (previous != OTBM::Token::Escape)
    {
      if (*cursor == OTBM::Token::Start)
        ++depth;
      if (*cursor == OTBM::Token::End)
        --depth;
    }

    previous = *cursor;
    ++cursor;
  }

  if (remainAtEnd)
  {
    --cursor;
  }
}

Position LoadBuffer::readPosition()
{
  uint16_t x = nextU16();
  uint16_t y = nextU16();
  uint8_t z = nextU8();

  return Position(x, y, z);
}

//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>Deserializer>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>

std::unique_ptr<OTBM::Deserializer> OTBM::Deserializer::create(OTBMVersion version, LoadBuffer &buffer)
{
  switch (version)
  {
  case OTBMVersion::OTBM1:
    return std::make_unique<OTBM::OTBM1Deserializer>(buffer);
  case OTBMVersion::OTBM2:
    return std::make_unique<OTBM::OTBM2Deserializer>(buffer);
  case OTBMVersion::OTBM3:
    return std::make_unique<OTBM::OTBM3Deserializer>(buffer);
  case OTBMVersion::OTBM4:
    return std::make_unique<OTBM::OTBM4Deserializer>(buffer);
  default:
    return {};
  }
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>OTBM1Deserializer>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>

void OTBM::OTBM1Deserializer::logWarning(std::string message)
{
  VME_LOG("[OTBM::DefaultDeserializer warning] " << message);
}

Item OTBM::OTBM1Deserializer::deserializeCompactItem()
{
  uint16_t id = buffer.nextU16();
  return Item(id);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>OTBM1Deserializer>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>

Item OTBM::OTBM1Deserializer::deserializeItem()
{
  uint16_t id = buffer.nextU16();
  Item item(id);

  deserializeItemAttributes(item);

  return item;
}

std::optional<std::string> OTBM::OTBM1Deserializer::deserializeItemAttributes(Item &item)
{
  while (buffer.peek() != OTBM::Token::End)
  {
    uint8_t attribute = buffer.nextU8();
    switch (static_cast<OTBM::NodeAttribute>(attribute))
    {
    case OTBM::NodeAttribute::Count:
      item.setSubtype(buffer.nextU8());
      break;
    case OTBM::NodeAttribute::ActionId:
      item.setActionId(buffer.nextU16());
      break;
    case OTBM::NodeAttribute::UniqueId:
      item.setUniqueId(buffer.nextU16());
      break;
    case OTBM::NodeAttribute::Charges:
      item.setCharges(buffer.nextU8());
      break;
    case OTBM::NodeAttribute::Text:
      item.setText(buffer.nextString());
      break;
    case OTBM::NodeAttribute::Desc:
      item.setDescription(buffer.nextString());
      break;
    case OTBM::NodeAttribute::RuneCharges:
      item.setCharges(buffer.nextU8());
      break;
    case OTBM::NodeAttribute::TeleportDestination:
    {
      Position destination = buffer.readPosition();
      ItemData::Teleport data(destination);

      item.setItemData(std::move(data));
      break;
    }
    case OTBM::NodeAttribute::HouseDoorId:
    {
      uint8_t doorId = buffer.nextU8();
      ItemData::HouseDoor data(doorId);

      item.setItemData(std::move(data));
      break;
    }
    case OTBM::NodeAttribute::DepotId:
    {
      uint16_t depotId = buffer.nextU16();
      ItemData::Depot data(depotId);

      item.setItemData(std::move(data));
      break;
    }
    default:
      ABORT_PROGRAM("Unknown attribute: " + attribute);
    }
  }

  return std::nullopt;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>OTBM4Deserializer>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>

std::optional<std::string> OTBM::OTBM4Deserializer::deserializeItemAttributes(Item &item)
{
  while (buffer.peek() != OTBM::Token::End)
  {
    uint8_t attribute = buffer.nextU8();
    switch (static_cast<OTBM::NodeAttribute>(attribute))
    {
    case OTBM::NodeAttribute::AttributeMap:
    {
      uint16_t amount = buffer.nextU16();
      for (uint16_t i = 0; i < amount; ++i)
      {
        std::string key = buffer.nextString();
        auto attributeType = ItemAttribute::parseAttributeString(key);
        if (!attributeType)
        {
          return "Unsupported Item attribute key: " + key;
        }

        ItemAttribute attribute(attributeType.value());
        auto attributeTypeId = static_cast<OTBM::AttributeTypeId>(buffer.nextU8());
        switch (attributeTypeId)
        {
        case OTBM::AttributeTypeId::String:
          attribute.setString(buffer.nextLongString());
          break;
        case OTBM::AttributeTypeId::Boolean:
          attribute.setBool(buffer.nextU8());
          break;
        case OTBM::AttributeTypeId::Integer:
          attribute.setInt(buffer.nextU32());
          break;
        case OTBM::AttributeTypeId::Double:
          attribute.setDouble(static_cast<double>(buffer.nextU64()));
          break;
        default:
          return "Unknown OTBM::AttributeTypeId: " + to_underlying(attributeTypeId);
        }
      }
    }
    }
  }

  return std::nullopt;
}
