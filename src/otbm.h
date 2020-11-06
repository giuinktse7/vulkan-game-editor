#pragma once

namespace OTBM
{

// Pragma pack is VERY important since otherwise it won't be able to load the structs correctly
#pragma pack(1)

  enum class NodeAttribute
  {
    Description = 1,
    ExternalFile = 2,
    TileFlags = 3,
    ActionId = 4,
    UniqueId = 5,
    Text = 6,
    Desc = 7,
    TeleportDestination = 8,
    Item = 9,
    Depot_id = 10,
    ExternalSpawnFile = 11,
    Rune_charges = 12,
    ExternalHouseFile = 13,
    HouseDoorId = 14,
    Count = 15,
    Duration = 16,
    DecayingState = 17,
    WrittenDate = 18,
    WrittenBy = 19,
    SleeperGuid = 20,
    SleepStart = 21,
    Charges = 22,

    AttributeMap = 128
  };

  enum class Node_t
  {
    Root = 0,
    Rootv1 = 1,
    MapData = 2,
    ItemDef = 3,
    TileArea = 4,
    Tile = 5,
    Item = 6,
    TileSquare = 7,
    TileRef = 8,
    Spawns = 9,
    SpawnArea = 10,
    Monster = 11,
    Towns = 12,
    Town = 13,
    Housetile = 14,
    Waypoints = 15,
    Waypoint = 16,
  };

  struct RootHeader
  {
    uint32_t version;
    uint16_t width;
    uint16_t height;
    uint32_t majorVersionItems;
    uint32_t minorVersionItems;
  };

  struct TeleportDest
  {
    uint16_t x;
    uint16_t y;
    uint8_t z;
  };

  struct TileAreaCoordinate
  {
    uint16_t x;
    uint16_t y;
    uint8_t z;
  };

  struct TileCoordinate
  {
    uint8_t x;
    uint8_t y;
  };

  struct TownTemple
  {
    uint16_t x;
    uint16_t y;
    uint8_t z;
  };

  struct HouseTile
  {
    uint8_t x;
    uint8_t y;
    uint32_t houseid;
  };

#pragma pack()

  enum class AttributeTypeId
  {
    String = 1,
    Integer = 2,
    Float = 3,
    Boolean = 4,
    Double = 5,
    None = 0
  };
} // namespace OTBM