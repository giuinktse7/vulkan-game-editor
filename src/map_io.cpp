#include "map_io.h"

#include <iostream>
#include <optional>
#include <string>

#include "debug.h"
#include "version.h"
#include "definitions.h"
#include "tile.h"

#pragma warning(push)
#pragma warning(disable : 26812)

enum NodeType
{
  NODE_START = 0xFE,
  NODE_END = 0xFF,
  ESCAPE_CHAR = 0xFD,
};

constexpr uint32_t DEFAULT_BUFFER_SIZE = 0xFFFF;

SaveBuffer::SaveBuffer(std::ofstream &stream)
    : stream(stream), maxBufferSize(DEFAULT_BUFFER_SIZE)
{
  buffer.reserve(DEFAULT_BUFFER_SIZE);
}

void SaveBuffer::writeBytes(const uint8_t *cursor, size_t amount)
{
  while (amount > 0)
  {
    if (*cursor == NODE_START || *cursor == NODE_END || *cursor == ESCAPE_CHAR)
    {
      if (buffer.size() + 1 >= maxBufferSize)
      {
        flushToFile();
      }
      buffer.emplace_back(ESCAPE_CHAR);
      std::cout << std::hex << static_cast<int>(buffer.back()) << std::endl;
    }

    if (buffer.size() + 1 >= maxBufferSize)
    {
      flushToFile();
    }
    buffer.emplace_back(*cursor);
    std::cout << std::hex << static_cast<int>(buffer.back()) << std::endl;

    ++cursor;
    --amount;
  }
}

void SaveBuffer::startNode(OTBM_NodeTypes_t nodeType)
{
  if (buffer.size() + 2 >= maxBufferSize)
  {
    flushToFile();
  }

  buffer.emplace_back(NODE_START);
  std::cout << std::hex << static_cast<int>(NODE_START) << std::endl;

  buffer.emplace_back(nodeType);
  std::cout << std::hex << static_cast<int>(nodeType) << std::endl;
}

void SaveBuffer::endNode()
{
  if (buffer.size() + 1 >= maxBufferSize)
  {
    flushToFile();
  }

  buffer.emplace_back(NODE_END);
  std::cout << std::hex << static_cast<int>(NODE_END) << std::endl;
}

void SaveBuffer::writeU8(uint8_t value)
{
  writeBytes(&value, 1);
}

void SaveBuffer::writeU16(uint16_t value)
{
  writeBytes(reinterpret_cast<uint8_t *>(&value), 2);
}

void SaveBuffer::writeU32(uint32_t value)
{
  writeBytes(reinterpret_cast<uint8_t *>(&value), 4);
}

void SaveBuffer::writeU64(uint64_t value)
{
  writeBytes(reinterpret_cast<uint8_t *>(&value), 8);
}

void SaveBuffer::writeString(const std::string &s)
{
  if (s.size() > UINT16_MAX)
  {
    ABORT_PROGRAM("OTBM does not support strings larger than 65535 bytes.");
  }

  writeU16(static_cast<uint16_t>(s.size()));
  writeBytes(reinterpret_cast<uint8_t *>(const_cast<char *>(s.data())), s.size());
}

void SaveBuffer::writeRawString(const std::string &s)
{
  writeBytes(reinterpret_cast<uint8_t *>(const_cast<char *>(s.data())), s.size());
}

void SaveBuffer::writeLongString(const std::string &s)
{
  if (s.size() > UINT32_MAX)
  {
    ABORT_PROGRAM("OTBM does not support long strings larger than 2^32 bytes.");
  }

  writeU32(static_cast<uint32_t>(s.size()));
  writeBytes(reinterpret_cast<uint8_t *>(const_cast<char *>(s.data())), s.size());
}

void SaveBuffer::flushToFile()
{
  std::cout << "flushToFile()" << std::endl;
  stream.write(reinterpret_cast<const char *>(buffer.data()), buffer.size());
  buffer.clear();
}

void SaveBuffer::finish()
{
  flushToFile();
}

void MapIO::saveMap(Map &map)
{
  std::ofstream stream;
  SaveBuffer buffer = SaveBuffer(stream);

  stream.open("map2.otbm", std::ofstream::out | std::ios::binary | std::ofstream::trunc);

  buffer.writeRawString("OTBM");

  buffer.startNode(OTBM_ROOT);
  {
    OTBMVersion otbmVersion = map.getMapVersion().otbmVersion;
    buffer.writeU32(static_cast<uint32_t>(otbmVersion));

    buffer.writeU16(map.getWidth());
    buffer.writeU16(map.getHeight());

    buffer.writeU32(Items::items.getOtbVersionInfo().majorVersion);
    buffer.writeU32(Items::items.getOtbVersionInfo().minorVersion);

    buffer.startNode(OTBM_MAP_DATA);
    {
      buffer.writeU8(OTBM_ATTR_DESCRIPTION);
      buffer.writeString("Saved by VME (Vulkan Map Editor)" + __VME_VERSION__);

      buffer.writeU8(OTBM_ATTR_DESCRIPTION);
      buffer.writeString(map.getDescription());

      buffer.writeU8(OTBM_ATTR_EXT_SPAWN_FILE);
      buffer.writeString("map.spawn.xml");

      buffer.writeU8(OTBM_ATTR_EXT_HOUSE_FILE);
      buffer.writeString("map.house.xml");

      // Tiles
      uint32_t savedTiles = 0;

      int x = -1;
      int y = -1;
      int z = -1;

      bool emptyMap = true;

      Serializer serializer(buffer, map.getMapVersion());

      for (const auto &location : map.begin())
      {
        ++savedTiles;

        Tile *tile = location->getTile();

        // We can skip the tile if it has no entities
        if (!tile || tile->getEntityCount() == 0)
        {
          continue;
        }

        const Position &pos = location->position();

        // Need new node?
        if (pos.x < x || pos.x > x + 0xFF || pos.y < y || pos.y > y + 0xFF || pos.z != z)
        {
          if (!emptyMap)
          {
            buffer.endNode();
          }
          emptyMap = false;

          buffer.startNode(OTBM_TILE_AREA);

          x = pos.x & 0xFF00;
          buffer.writeU16(x);

          y = pos.y & 0xFF00;
          buffer.writeU16(y);

          z = pos.z;
          buffer.writeU8(z);
        }

        bool isHouseTile = false;
        buffer.startNode(isHouseTile ? OTBM_HOUSETILE : OTBM_TILE);

        buffer.writeU8(location->x() & 0xFF);
        buffer.writeU8(location->y() & 0xFF);

        if (isHouseTile)
        {
          uint32_t houseId = 0;
          buffer.writeU32(houseId);
        }

        if (tile->getMapFlags())
        {
          buffer.writeU8(OTBM_ATTR_TILE_FLAGS);
          buffer.writeU32(tile->getMapFlags());
        }

        if (tile->getGround())
        {
          Item *ground = tile->getGround();
          if (ground->hasAttributes())
          {
            serializer.serializeItem(*ground);
          }
          else
          {
            buffer.writeU16(ground->getId());
          }
        }

        for (const Item &item : tile->getItems())
        {
          serializer.serializeItem(item);
        }

        buffer.endNode();
      }

      // Close the last node
      if (!emptyMap)
      {
        buffer.endNode();
      }

      buffer.startNode(OTBM_TOWNS);
      for (auto &townEntry : map.getTowns())
      {
        Town &town = townEntry.second;
        const Position &townPos = town.getTemplePosition();
        buffer.startNode(OTBM_TOWN);

        buffer.writeU32(town.getID());
        buffer.writeString(town.getName());
        buffer.writeU16(townPos.x);
        buffer.writeU16(townPos.y);
        buffer.writeU8(townPos.z);

        buffer.endNode();
      }
      buffer.endNode();

      if (otbmVersion >= OTBMVersion::MAP_OTBM_3)
      {
        // TODO write waypoints
        // TODO See RME: iomap_otb.cpp line 1415
      }
    }

    buffer.endNode();
  }
  buffer.endNode();

  buffer.finish();

  stream.close();
}

void MapIO::Serializer::serializeItem(const Item &item)
{
  buffer.startNode(OTBM_ITEM);
  buffer.writeU16(item.getId());

  serializeItemAttributes(item);

  buffer.endNode();
}

void MapIO::Serializer::serializeItemAttributes(const Item &item)
{
  if (mapVersion.otbmVersion >= OTBMVersion::MAP_OTBM_2)
  {
    const ItemType &itemType = *item.itemType;
    if (itemType.usesSubType())
    {
      buffer.writeU8(OTBM_ATTR_COUNT);
      buffer.writeU8(item.getSubtype());
    }
  }

  if (mapVersion.otbmVersion >= OTBMVersion::MAP_OTBM_4)
  {
    if (item.hasAttributes())
    {
      buffer.writeU8(static_cast<uint8_t>(OTBM_ATTR_ATTRIBUTE_MAP));
      serializeItemAttributeMap(item.getAttributes());
    }
  }
}

void MapIO::Serializer::serializeItemAttributeMap(const std::unordered_map<ItemAttribute_t, ItemAttribute> &attributes)
{
  // Can not have more than UINT16_MAX items
  buffer.writeU16(static_cast<uint16_t>(std::min((size_t)UINT16_MAX, attributes.size())));

  auto entry = attributes.begin();

  int i = 0;
  while (entry != attributes.end() && i <= UINT16_MAX)
  {
    const ItemAttribute_t &attributeType = entry->first;
    std::stringstream attributeTypeString;
    attributeTypeString << attributeType;
    std::string s = attributeTypeString.str();
    if (s.size() > UINT16_MAX)
    {
      buffer.writeString(s.substr(0, UINT16_MAX));
    }
    else
    {
      buffer.writeString(s);
    }

    auto attribute = entry->second;
    serializeItemAttribute(attribute);
    ++entry;
    ++i;
  }
}

void MapIO::Serializer::serializeItemAttribute(ItemAttribute &attribute)
{
  buffer.writeU8(static_cast<uint8_t>(attribute.type));

  if (attribute.holds<std::string>())
  {
    auto k = attribute.get<std::string>();
    auto s = k.value();
    buffer.writeLongString(s);
  }
  else if (attribute.holds<int>())
  {
    buffer.writeU32(static_cast<uint32_t>(attribute.get<int>().value()));
  }
  else if (attribute.holds<double>())
  {
    buffer.writeU64(static_cast<uint64_t>(attribute.get<double>().value()));
  }
}

#pragma warning(pop)