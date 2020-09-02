#pragma once

#include <fstream>
#include <vector>
#include <unordered_map>

#include "map.h"
#include "item.h"

#include "item_attribute.h"

// Pragma pack is VERY important since otherwise it won't be able to load the structs correctly
#pragma pack(1)

enum OTBM_ItemAttribute
{
	OTBM_ATTR_DESCRIPTION = 1,
	OTBM_ATTR_EXT_FILE = 2,
	OTBM_ATTR_TILE_FLAGS = 3,
	OTBM_ATTR_ACTION_ID = 4,
	OTBM_ATTR_UNIQUE_ID = 5,
	OTBM_ATTR_TEXT = 6,
	OTBM_ATTR_DESC = 7,
	OTBM_ATTR_TELE_DEST = 8,
	OTBM_ATTR_ITEM = 9,
	OTBM_ATTR_DEPOT_ID = 10,
	OTBM_ATTR_EXT_SPAWN_FILE = 11,
	OTBM_ATTR_RUNE_CHARGES = 12,
	OTBM_ATTR_EXT_HOUSE_FILE = 13,
	OTBM_ATTR_HOUSEDOORID = 14,
	OTBM_ATTR_COUNT = 15,
	OTBM_ATTR_DURATION = 16,
	OTBM_ATTR_DECAYING_STATE = 17,
	OTBM_ATTR_WRITTENDATE = 18,
	OTBM_ATTR_WRITTENBY = 19,
	OTBM_ATTR_SLEEPERGUID = 20,
	OTBM_ATTR_SLEEPSTART = 21,
	OTBM_ATTR_CHARGES = 22,

	OTBM_ATTR_ATTRIBUTE_MAP = 128
};

enum OTBM_NodeTypes_t
{
	OTBM_ROOT = 0,
	OTBM_ROOTV1 = 1,
	OTBM_MAP_DATA = 2,
	OTBM_ITEM_DEF = 3,
	OTBM_TILE_AREA = 4,
	OTBM_TILE = 5,
	OTBM_ITEM = 6,
	OTBM_TILE_SQUARE = 7,
	OTBM_TILE_REF = 8,
	OTBM_SPAWNS = 9,
	OTBM_SPAWN_AREA = 10,
	OTBM_MONSTER = 11,
	OTBM_TOWNS = 12,
	OTBM_TOWN = 13,
	OTBM_HOUSETILE = 14,
	OTBM_WAYPOINTS = 15,
	OTBM_WAYPOINT = 16,
};

struct OTBM_root_header
{
	uint32_t version;
	uint16_t width;
	uint16_t height;
	uint32_t majorVersionItems;
	uint32_t minorVersionItems;
};

struct OTBM_TeleportDest
{
	uint16_t x;
	uint16_t y;
	uint8_t z;
};

struct OTBM_Tile_area_coords
{
	uint16_t x;
	uint16_t y;
	uint8_t z;
};

struct OTBM_Tile_coords
{
	uint8_t x;
	uint8_t y;
};

struct OTBM_TownTemple_coords
{
	uint16_t x;
	uint16_t y;
	uint8_t z;
};

struct OTBM_HouseTile_coords
{
	uint8_t x;
	uint8_t y;
	uint32_t houseid;
};

#pragma pack()

/*
Small wrapper for a buffer that is written to when saving an OTBM map.
*/
class SaveBuffer
{
public:
	SaveBuffer(std::ofstream &stream);

	void writeU8(uint8_t value);
	void writeU16(uint16_t value);
	void writeU32(uint32_t value);
	void writeU64(uint64_t value);
	void writeString(const std::string &s);
	void writeLongString(const std::string &s);
	void writeRawString(const std::string &s);

	void startNode(OTBM_NodeTypes_t value);
	void endNode();

	void finish();

private:
	std::ostream &stream;
	std::vector<uint8_t> buffer;

	size_t maxBufferSize;

	void writeBytes(const uint8_t *start, size_t amount);
	void flushToFile();
};

namespace MapIO
{
	void saveMap(Map &map);

	class Serializer
	{
	public:
		Serializer(SaveBuffer &buffer, MapVersion mapVersion)
				: buffer(buffer), mapVersion(mapVersion) {}
		void serializeItem(const Item &item);
		void serializeItemAttributes(const Item &item);
		void serializeItemAttributeMap(const std::unordered_map<ItemAttribute_t, ItemAttribute> &attributes);
		void serializeItemAttribute(ItemAttribute &attribute);

	private:
		MapVersion &mapVersion;
		SaveBuffer &buffer;
	};

} // namespace MapIO