#include "items.h"

#include <iostream>
#include <utility>
#include <memory>
#include <algorithm>
#include <bitset>
#include "logger.h"

#include "version.h"
#include "otb.h"
#include "util.h"

#include "graphics/appearances.h"
#include "graphics/engine.h"
#include "item_type.h"
#include "time_point.h"

Items Items::items;

constexpr uint32_t RESERVED_ITEM_COUNT = 30000;

using std::string;

constexpr auto OTBI = OTB::Identifier{{'O', 'T', 'B', 'I'}};

Items::Items()
{
}

ItemType *Items::getItemType(uint16_t id)
{
	if (id >= itemTypes.size())
		return nullptr;

	return &itemTypes.at(id);
}

ItemType *Items::getItemTypeByClientId(uint16_t clientId)
{
	uint16_t id = clientIdToServerId.at(clientId);
	if (id >= itemTypes.size())
		return nullptr;

	return &itemTypes.at(id);
}

void Items::loadFromXml(const std::filesystem::path path)
{
	TimePoint start;

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(path.c_str());

	if (!result)
	{
		std::string pugiError = result.description();
		ABORT_PROGRAM("Could not load items.xml (Syntax error?): " + pugiError);
	}

	pugi::xml_node node = doc.child("items");
	if (!node)
	{
		ABORT_PROGRAM("items.xml, invalid root node.");
	}

	for (pugi::xml_node itemNode = node.first_child(); itemNode; itemNode = itemNode.next_sibling())
	{
		if (as_lower_str(itemNode.name()) != "item")
		{
			continue;
		}

		uint32_t fromId = itemNode.attribute("fromid").as_uint();
		uint32_t toId = itemNode.attribute("toid").as_uint();
		if (pugi::xml_attribute attribute = itemNode.attribute("id"))
		{
			fromId = toId = attribute.as_int();
		}

		if (fromId == 0 || toId == 0)
		{
			ABORT_PROGRAM("Could not read item id from item node.");
		}

		for (uint32_t id = fromId; id <= toId; ++id)
		{
			loadItemFromXml(itemNode, id);
		}
	}

	Logger::info() << "Loaded items.xml in " << start.elapsedMillis() << " ms." << std::endl;
}

bool Items::loadItemFromXml(pugi::xml_node itemNode, uint32_t id)
{
	// TODO Fix versioning
	// TODO Why do we skip these in these cases?
	int clientVersion = 52;
	if (clientVersion < to_underlying(ClientVersion::CLIENT_VERSION_980) && id > 20000 && id < 20100)
	{
		itemNode = itemNode.next_sibling();
		return {};
	}
	else if (id > 30000 && id < 30100)
	{
		itemNode = itemNode.next_sibling();
		return {};
	}

	if (!Items::items.hasItemType(id))
	{
		// Fluids in current version, might change in the future
		if (id >= 40001 && id <= 40043)
		{
			return {};
		}
		Logger::error("There is no itemType with server ID " + std::to_string(id));
		return false;
	}

	ItemType &it = *Items::items.getItemType(id);

	it.name = itemNode.attribute("name").as_string();
	it.editorsuffix = itemNode.attribute("editorsuffix").as_string();

	pugi::xml_attribute attribute;
	for (pugi::xml_node itemAttributesNode = itemNode.first_child(); itemAttributesNode; itemAttributesNode = itemAttributesNode.next_sibling())
	{
		if (!(attribute = itemAttributesNode.attribute("key")))
		{
			continue;
		}

		std::string key = attribute.as_string();
		to_lower_str(key);
		if (key == "type")
		{
			if (!(attribute = itemAttributesNode.attribute("value")))
			{
				continue;
			}

			std::string typeValue = attribute.as_string();
			to_lower_str(key);
			if (typeValue == "depot")
			{
				it.type = ITEM_TYPE_DEPOT;
			}
			else if (typeValue == "mailbox")
			{
				it.type = ITEM_TYPE_MAILBOX;
			}
			else if (typeValue == "trashholder")
			{
				it.type = ITEM_TYPE_TRASHHOLDER;
			}
			else if (typeValue == "container")
			{
				it.type = ITEM_TYPE_CONTAINER;
			}
			else if (typeValue == "door")
			{
				it.type = ITEM_TYPE_DOOR;
			}
			else if (typeValue == "magicfield")
			{
				it.group = ITEM_GROUP_MAGICFIELD;
				it.type = ITEM_TYPE_MAGICFIELD;
			}
			else if (typeValue == "teleport")
			{
				it.type = ITEM_TYPE_TELEPORT;
			}
			else if (typeValue == "bed")
			{
				it.type = ITEM_TYPE_BED;
			}
			else if (typeValue == "key")
			{
				it.type = ITEM_TYPE_KEY;
			}
		}
		else if (key == "name")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.name = attribute.as_string();
			}
		}
		else if (key == "description")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.description = attribute.as_string();
			}
		}
		else if (key == "runespellName")
		{
			/*if((attribute = itemAttributesNode.attribute("value"))) {
				it.runeSpellName = attribute.as_string();
			}*/
		}
		else if (key == "weight")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.weight = static_cast<uint32_t>(std::floor(attribute.as_int() / 100.f));
			}
		}
		else if (key == "armor")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.armor = attribute.as_int();
			}
		}
		else if (key == "defense")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.defense = attribute.as_int();
			}
		}
		else if (key == "rotateto")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.rotateTo = attribute.as_int();
			}
		}
		else if (key == "containersize")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.volume = attribute.as_int();
			}
		}
		else if (key == "readable")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.canReadText = attribute.as_bool();
			}
		}
		else if (key == "writeable")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.canWriteText = it.canReadText = attribute.as_bool();
			}
		}
		else if (key == "decayto")
		{
			it.decays = true;
		}
		else if (key == "maxtextlen" || key == "maxtextlength")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.maxTextLen = attribute.as_int();
				it.canReadText = it.maxTextLen > 0;
			}
		}
		else if (key == "writeonceitemid")
		{
			/*if((attribute = itemAttributesNode.attribute("value"))) {
				it.writeOnceItemId = pugi::cast<int32_t>(attribute.value());
			}*/
		}
		else if (key == "allowdistread")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.allowDistRead = attribute.as_bool();
			}
		}
		else if (key == "charges")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				it.charges = attribute.as_int();
			}
		}
		else if (key == "floorchange")
		{
			if ((attribute = itemAttributesNode.attribute("value")))
			{
				std::string value = attribute.as_string();
				if (value == "down")
				{
					it.floorChange = FloorChange::Down;
				}
				else if (value == "north")
				{
					it.floorChange = FloorChange::North;
				}
				else if (value == "south")
				{
					it.floorChange = FloorChange::South;
				}
				else if (value == "west")
				{
					it.floorChange = FloorChange::West;
				}
				else if (value == "east")
				{
					it.floorChange = FloorChange::East;
				}
				else if (value == "northex")
					it.floorChange = FloorChange::NorthEx;
				else if (value == "southex")
					it.floorChange = FloorChange::SouthEx;
				else if (value == "westex")
					it.floorChange = FloorChange::WestEx;
				else if (value == "eastex")
					it.floorChange = FloorChange::EastEx;
				else if (value == "southalt")
					it.floorChange = FloorChange::SouthAlt;
				else if (value == "eastalt")
					it.floorChange = FloorChange::EastAlt;
			}
		}
	}
	return true;
}

void Items::loadFromOtb(const std::string &file)
{
	TimePoint start;

	if (!Appearances::isLoaded)
	{
		throw std::runtime_error("Appearances must be loaded before loading items.otb.");
	}

	Items &items = Items::items;

	// OTB::Loader loader{file, OTBI};
	std::unique_ptr<OTB::Loader> loader = std::make_unique<OTB::Loader>(file, OTBI);

	auto &root = loader->parseTree();

	PropStream props;

	items.itemTypes.reserve(RESERVED_ITEM_COUNT);
	items.clientIdToServerId.reserve(RESERVED_ITEM_COUNT);
	// items.serverIdToMapId.reserve(RESERVED_ITEM_COUNT);
	items.nameToItems.reserve(RESERVED_ITEM_COUNT);

	if (loader->getProps(root, props))
	{
		//4 byte flags
		//attributes
		//0x01 = version data
		uint32_t flags;
		if (!props.read<uint32_t>(flags))
		{
			ABORT_PROGRAM("Could not read the OTB header flags.");
		}

		uint8_t attr;
		if (!props.read<uint8_t>(attr))
		{
			ABORT_PROGRAM("Could not read the OTB header attr.");
		}

		if (attr == to_underlying(OTB::RootAttributes::Version))
		{
			uint16_t length;
			if (!props.read<uint16_t>(length))
			{
				ABORT_PROGRAM("Could not read the OTB version length.");
			}

			if (length != sizeof(OTB::VersionInfo))
			{
				ABORT_PROGRAM("The length of the version info is incorrect.");
			}

			if (!props.read(items.otbVersionInfo))
			{
				ABORT_PROGRAM("Could not read OTB version information.");
			}
		}
	}

	if (items.otbVersionInfo.majorVersion == 0xFFFFFFFF)
	{
		std::cout << "[Warning - Items::loadFromOtb] items.otb using generic client version." << std::endl;
	}
	else if (items.otbVersionInfo.majorVersion != 3)
	{
		ABORT_PROGRAM("Old version detected, a newer version of items.otb is required.");
	}
	else if (items.otbVersionInfo.minorVersion < to_underlying(ClientVersion::CLIENT_VERSION_1098))
	{
		ABORT_PROGRAM("A newer version of items.otb is required.");
	}

	// Flag to skip item if a recoverable problem occurs
	bool skipItem = false;
	for (auto &itemNode : root.children)
	{
		skipItem = false;
		PropStream stream;
		if (!loader->getProps(itemNode, stream))
		{
			ABORT_PROGRAM("Could not get props for a node.");
		}

		uint32_t flags;
		if (!stream.read<uint32_t>(flags))
		{
			ABORT_PROGRAM("Could not read flags for a node.");
		}

		uint16_t serverId = 0;
		uint16_t clientId = 0;
		uint16_t speed = 0;
		uint16_t wareId = 0;
		uint8_t lightLevel = 0;
		uint8_t lightColor = 0;
		uint8_t alwaysOnTopOrder = 0;

		uint8_t attrib;

		const auto DEFAULT_ERROR = "Bad format.";

		while (stream.read<uint8_t>(attrib))
		{
			uint16_t length;
			if (!stream.read<uint16_t>(length))
			{
				ABORT_PROGRAM("Could not read attribute length.");
			}

			switch (attrib)
			{
			case itemproperty_t::ITEM_ATTR_SERVERID:
			{
				if (length != sizeof(uint16_t))
				{
					ABORT_PROGRAM("Invalid attribute length.");
				}

				if (!stream.read<uint16_t>(serverId))
				{
					ABORT_PROGRAM("Could not read server ID.");
				}

				if (serverId > 30000 && serverId < 30100)
				{
					serverId -= 30000;
				}

				break;
			}

			case itemproperty_t::ITEM_ATTR_CLIENTID:
			{
				if (length != sizeof(uint16_t))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}

				if (!stream.read<uint16_t>(clientId))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}
				break;
			}

			case itemproperty_t::ITEM_ATTR_SPEED:
			{
				if (length != sizeof(uint16_t))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}

				if (!stream.read<uint16_t>(speed))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}
				break;
			}

			case itemproperty_t::ITEM_ATTR_LIGHT2:
			{
				OTB::LightInfo lightInfo;
				if (length != sizeof(OTB::LightInfo))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}

				if (!stream.read(lightInfo))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}

				lightLevel = static_cast<uint8_t>(lightInfo.level);
				lightColor = static_cast<uint8_t>(lightInfo.color);
				break;
			}

			case itemproperty_t::ITEM_ATTR_TOPORDER:
			{
				if (length != sizeof(uint8_t))
				{
					ABORT_PROGRAM("Bad length.");
				}

				if (!stream.read<uint8_t>(alwaysOnTopOrder))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}
				break;
			}

			case itemproperty_t::ITEM_ATTR_WAREID:
			{
				if (length != sizeof(uint16_t))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}

				if (!stream.read<uint16_t>(wareId))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}
				break;
			}

			default:
			{
				//skip unknown attributes
				if (!stream.skip(length))
				{
					ABORT_PROGRAM(DEFAULT_ERROR);
				}
				break;
			}
			}
		}

		items.clientIdToServerId.emplace(clientId, serverId);

		// store the found item
		if (serverId >= items.itemTypes.size())
		{
			size_t arbitrarySizeIncrease = 2000;
			items.itemTypes.resize(static_cast<size_t>(serverId) + arbitrarySizeIncrease);
		}

		ItemType &iType = items.itemTypes[serverId];

		if (!Appearances::hasObject(clientId))
		{
			iType.clientId = 0;

			continue;
		}

		// items.serverIdToMapId.emplace(serverId, mapId);

		auto &appearance = Appearances::getObjectById(clientId);
		iType.appearance = &appearance;
		iType.cacheTextureAtlases();
		iType.name = appearance.name;

		iType.group = static_cast<itemgroup_t>(itemNode.type);
		switch (itemNode.type)
		{
		case itemgroup_t::ITEM_GROUP_CONTAINER:
			iType.type = ITEM_TYPE_CONTAINER;
			break;
		case ITEM_GROUP_DOOR:
			//not used
			iType.type = ITEM_TYPE_DOOR;
			break;
		case itemgroup_t::ITEM_GROUP_MAGICFIELD:
			//not used
			iType.type = ItemTypes_t::ITEM_TYPE_MAGICFIELD;
			break;
		case itemgroup_t::ITEM_GROUP_TELEPORT:
			//not used
			iType.type = ItemTypes_t::ITEM_TYPE_TELEPORT;
			break;
		case itemgroup_t::ITEM_GROUP_NONE:
		case itemgroup_t::ITEM_GROUP_GROUND:
		case itemgroup_t::ITEM_GROUP_SPLASH:
		case itemgroup_t::ITEM_GROUP_FLUID:
		case itemgroup_t::ITEM_GROUP_CHARGES:
		case itemgroup_t::ITEM_GROUP_DEPRECATED:
			break;
		default:
			ABORT_PROGRAM(DEFAULT_ERROR);
		}

		if (serverId == 4526 || serverId == 103)
		{
			Logger::debug() << serverId << ":" << std::endl;
			std::cout << appearance << std::endl;
		}

		// TODO: Check for items that do not have matching flags in .otb and appearances.dat
		iType.blockSolid = hasBitSet(FLAG_BLOCK_SOLID, flags);
		iType.blockProjectile = hasBitSet(FLAG_BLOCK_PROJECTILE, flags);
		iType.blockPathFind = hasBitSet(FLAG_BLOCK_PATHFIND, flags);
		iType.hasHeight = hasBitSet(FLAG_HAS_HEIGHT, flags);
		iType.useable = hasBitSet(FLAG_USEABLE, flags) || appearance.hasFlag(AppearanceFlag::Usable);
		iType.pickupable = hasBitSet(FLAG_PICKUPABLE, flags);
		iType.moveable = hasBitSet(FLAG_MOVEABLE, flags);
		iType.stackable = hasBitSet(FLAG_STACKABLE, flags);

		iType.alwaysOnTop = hasBitSet(FLAG_ALWAYSONTOP, flags);
		iType.isVertical = hasBitSet(FLAG_VERTICAL, flags);
		iType.isHorizontal = hasBitSet(FLAG_HORIZONTAL, flags);
		iType.isHangable = hasBitSet(FLAG_HANGABLE, flags);
		iType.allowDistRead = hasBitSet(FLAG_ALLOWDISTREAD, flags);
		iType.rotatable = hasBitSet(FLAG_ROTATABLE, flags);
		iType.canReadText = hasBitSet(FLAG_READABLE, flags);
		iType.lookThrough = hasBitSet(FLAG_LOOKTHROUGH, flags);
		iType.isAnimation = hasBitSet(FLAG_ANIMATION, flags);
		// iType.walkStack = !hasBitSet(FLAG_FULLTILE, flags);
		iType.forceUse = hasBitSet(FLAG_FORCEUSE, flags);

		iType.id = serverId;
		iType.clientId = clientId;
		iType.speed = speed;
		iType.lightLevel = lightLevel;
		iType.lightColor = lightColor;
		iType.wareId = wareId;
		iType.alwaysOnTopOrder = alwaysOnTopOrder;

		// #define CHECK_ITEM_FLAG(flag, prop)                                                                          \
// 	do                                                                                                         \
// 	{                                                                                                          \
// 		if (appearance.hasFlag(flag) != prop)                                                                    \
// 		{                                                                                                        \
// 			std::cout << "Here:" << std::endl                                                                      \
// 								<< serverId << #flag << "[" << appearance.hasFlag(flag) << ", " << prop << "]" << std::endl; \
// 			std::cout << std::bitset<32>(flags) << std::endl;                                                      \
// 		}                                                                                                        \
// 	} while (false)

		// 		CHECK_ITEM_FLAG(AppearanceFlag::Height, iType.hasHeight);
		// 		CHECK_ITEM_FLAG(AppearanceFlag::Usable, iType.useable);
		// 		CHECK_ITEM_FLAG(AppearanceFlag::Take, iType.pickupable);

		// #undef CHECK_ITEM_FLAG

		// if (appearance.hasFlag(AppearanceFlag::Shift))
		// {
		// 	std::cout << serverId << ": (x=" << appearance.flagData.shiftX << ", y=" << appearance.flagData.shiftY << ")" << std::endl;
		// }
	}

	items.itemTypes.shrink_to_fit();

	Logger::info() << "Loaded items.otb in " << start.elapsedMillis() << " ms." << std::endl;
}

ItemType *Items::getNextValidItemType(uint16_t serverId)
{
	ABORT_PROGRAM("Unimplemented! validItemTypeStartId has to be populated.");
	auto itemType = getItemType(serverId);
	if (itemType->isValid())
	{
		return itemType;
	}

	auto found = std::lower_bound(validItemTypeStartId.begin(), validItemTypeStartId.end(), serverId);

	if (found == validItemTypeStartId.end())
	{
		return nullptr;
	}

	return getItemType(*found);
}

ItemType *Items::getPreviousValidItemType(uint16_t serverId)
{
	ABORT_PROGRAM("Unimplemented! validItemTypeEndId has to be populated.");

	auto itemType = getItemType(serverId);
	if (itemType->isValid())
	{
		return itemType;
	}

	auto found = std::lower_bound(validItemTypeEndId.begin(), validItemTypeEndId.end(), serverId);

	if (found == validItemTypeEndId.begin())
	{
		return nullptr;
	}

	return getItemType(*(--found));
}

OTB::VersionInfo Items::getOtbVersionInfo()
{
	return otbVersionInfo;
}

const ItemType &Items::getItemIdByClientId(uint16_t spriteId) const
{
	return itemTypes.at(clientIdToServerId.at(spriteId));
}
