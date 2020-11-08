#include "items.h"

#include <algorithm>
#include <bitset>
#include <iostream>
#include <memory>
#include <utility>

#include "file.h"
#include "logger.h"
#include "otb.h"
#include "util.h"
#include "version.h"

#include "graphics/appearances.h"
#include "item_type.h"

Items Items::items;

namespace
{
	constexpr uint32_t ReservedItemCount = 40000;

	constexpr ClientVersion DefaultVersion = ClientVersion::CLIENT_VERSION_1098;

	bool reservedForFluid(uint16_t id, ClientVersion clientVersion) noexcept
	{
		auto version = to_underlying(clientVersion);

		if (version < to_underlying(ClientVersion::CLIENT_VERSION_980) && 20000 < id && id < 20100)
		{
			return true;
		}
		// TODO What is the version range for fluids in [30000, 30100]?
		// else if (id > 30000 && id < 30100) {}
		// Fluids in current version, might change in the future
		else if (40000 < id && id < 40100)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
} // namespace

using std::string;

Items::Items()
{
}

ItemType *Items::getItemType(uint32_t id)
{
	if (id >= itemTypes.size())
		return nullptr;

	return &itemTypes.at(id);
}

bool Items::validItemType(uint32_t serverId) const
{
	if (serverId >= itemTypes.size())
		return false;

	return itemTypes.at(serverId).appearance != nullptr;
}

ItemType *Items::getItemTypeByClientId(uint32_t clientId)
{
	uint32_t id = clientIdToServerId.at(clientId);
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

	VME_LOG("Loaded items.xml in " << start.elapsedMillis() << " ms.");
}

bool Items::loadItemFromXml(pugi::xml_node itemNode, uint32_t id)
{
	if (!Items::items.validItemType(id) || reservedForFluid(id, DefaultVersion))
		return false;

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
			to_lower_str(typeValue);
			if (typeValue == "depot")
			{
				it.type = ItemTypes_t::Depot;
			}
			else if (typeValue == "mailbox")
			{
				it.type = ItemTypes_t::Mailbox;
			}
			else if (typeValue == "trashholder")
			{
				it.type = ItemTypes_t::TrashHolder;
			}
			else if (typeValue == "container")
			{
				it.type = ItemTypes_t::Container;
			}
			else if (typeValue == "door")
			{
				it.type = ItemTypes_t::Door;
			}
			else if (typeValue == "magicfield")
			{
				it.group = itemgroup_t::MagicField;
				it.type = ItemTypes_t::MagicField;
			}
			else if (typeValue == "teleport")
			{
				it.type = ItemTypes_t::Teleport;
			}
			else if (typeValue == "bed")
			{
				it.type = ItemTypes_t::Bed;
			}
			else if (typeValue == "key")
			{
				it.type = ItemTypes_t::Key;
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

void Items::loadFromOtb(const std::filesystem::path path)
{
	OtbReader reader(path.string());
	reader.parseOTB();
}

void Items::OtbReader::parseOTB()
{
	this->start = TimePoint::now();

	if (!Appearances::isLoaded)
	{
		throw std::runtime_error("Appearances must be loaded before loading items.otb.");
	}

	readRoot();
}
void Items::OtbReader::readRoot()
{
	// Read first 4 empty bytes
	uint32_t value = nextU32();
	if (value != 0)
	{
		ABORT_PROGRAM("Expected first 4 bytes to be 0");
	}

	uint8_t start = nextU8();
	if (start != OtbReader::StartNode)
	{
		ABORT_PROGRAM("Expected Token::Start");
	}

	nextU8();	 // First byte of otb is zero
	nextU32(); // 4 unused bytes

	// Root Header Version
	uint8_t rootAttribute = nextU8();
	if (rootAttribute != OtbReader::OTBM_ROOTV1)
	{
		ABORT_PROGRAM("Unsupported RootHeaderVersion.");
	}

	uint16_t dataLength = nextU16();
	if (dataLength != sizeof(OTB::VersionInfo))
	{
		ABORT_PROGRAM("Size of version header is invalid.");
	}

	info.majorVersion = nextU32(); // items otb format file version
	info.minorVersion = nextU32(); // client version
	info.buildNumber = nextU32();	 // build number, revision

	// OTB description, something like 'OTB 3.62.78-11.1'
	std::vector<uint8_t> buffer = util::sliceLeading<uint8_t>(nextBytes(128), 0);
	description = std::string(buffer.begin(), buffer.end());

	readNodes();

	Items::items.itemTypes.shrink_to_fit();
	VME_LOG("Loaded items.otb in " << this->start.elapsedMillis() << " ms.");
}

Items::OtbReader::OtbReader(const std::string &file)
		: buffer(File::read(file))
{
	cursor = buffer.begin();
	path = file;
}

void Items::OtbReader::readNodes()
{
	Items &items = Items::items;
	items.itemTypes.resize(ReservedItemCount);
	items.clientIdToServerId.reserve(ReservedItemCount);
	items.nameToItems.reserve(ReservedItemCount);

	do
	{
		[[maybe_unused]] uint8_t startNode = nextU8();
		DEBUG_ASSERT(startNode == StartNode, "startNode must be OtbReader::StartNode.");

		// ItemType itemType;

		uint8_t groupByte = nextU8();
		uint32_t flags = nextU32();

		uint32_t serverId = 0;
		uint32_t clientId = 0;
		uint16_t speed = 0;
		uint16_t lightLevel = 0;
		uint16_t lightColor = 0;
		uint16_t alwaysOnTopOrder = 0;
		uint16_t wareId = 0;
		std::string name;
		uint16_t maxTextLen = 0;

		ItemType *itemType = nullptr;

		// Read attributes
		while (!nodeEnd())
		{
			uint8_t attributeType = nextU8();
			uint16_t attributeSize = nextU16();

			switch (static_cast<itemproperty_t>(attributeType))
			{

			case itemproperty_t::ITEM_ATTR_SERVERID:
			{
				DEBUG_ASSERT(attributeSize == sizeof(uint16_t), "Invalid attribute length.");
				serverId = nextU16();

				// Not sure why we do this. It is possibly ids reserved for fluid types.
				if (30000 < serverId && serverId < 30100)
				{
					serverId -= 30000;
				}

				itemType = &items.itemTypes[serverId];
				itemType->id = static_cast<uint32_t>(serverId);

				break;
			}

			case itemproperty_t::ITEM_ATTR_CLIENTID:
			{
				DEBUG_ASSERT(attributeSize == sizeof(uint16_t), "Invalid attribute length.");
				clientId = static_cast<uint32_t>(nextU16());
				break;
			}

			case itemproperty_t::ITEM_ATTR_SPEED:
			{
				DEBUG_ASSERT(attributeSize == sizeof(uint16_t), "Invalid attribute length.");
				speed = nextU16();
				break;
			}

			case itemproperty_t::ITEM_ATTR_LIGHT2:
			{
				DEBUG_ASSERT(attributeSize == sizeof(OTB::LightInfo), "Invalid attribute length.");

				lightLevel = nextU16();
				lightColor = nextU16();
				break;
			}

			case itemproperty_t::ITEM_ATTR_TOPORDER:
			{
				DEBUG_ASSERT(attributeSize == sizeof(uint8_t), "Invalid attribute length.");
				alwaysOnTopOrder = nextU8();
				break;
			}

			case itemproperty_t::ITEM_ATTR_WAREID:
			{
				DEBUG_ASSERT(attributeSize == sizeof(uint16_t), "Invalid attribute length.");
				wareId = nextU16();
				break;
			}

			case itemproperty_t::ITEM_ATTR_SPRITEHASH:
			{
				// Ignore spritehash
				skipBytes(attributeSize);
				break;
			}

			case itemproperty_t::ITEM_ATTR_MINIMAPCOLOR:
			{
				//Ignore it (get it from appearances as 'automapColor' instead)
				skipBytes(attributeSize);
				break;
			}

			case itemproperty_t::ITEM_ATTR_NAME:
			{
				if (itemType)
				{
					itemType->name = nextString(attributeSize);
				}
				else
				{
					name = nextString(attributeSize);
				}
				break;
			}

			case itemproperty_t::ITEM_ATTR_MAX_TEXT_LENGTH:
			{
				maxTextLen = nextU16();
				break;
			}

			case itemproperty_t::ITEM_ATTR_MAX_TEXT_LENGTH_ONCE:
			{
				maxTextLen = nextU16();
				break;
			}

			default:
			{
				//skip unknown attributes
				VME_LOG_D("Skipped unknown: " << static_cast<itemproperty_t>(attributeType));
				std::vector<uint8_t> s(cursor - 8, cursor);
				for (auto &k : s)
				{
					VME_LOG_D((int)k);
				}
				skipBytes(attributeSize);
				break;
			}
			}
		}
		if (itemType == nullptr)
		{
			ABORT_PROGRAM("Encountered item type without a server ID.");
		}

		itemType->group = static_cast<itemgroup_t>(groupByte);
		itemType->type = serverItemType(itemType->group);

		itemType->id = serverId;
		itemType->clientId = clientId;
		itemType->speed = speed;
		itemType->lightLevel = lightLevel;
		itemType->lightColor = lightColor;
		itemType->alwaysOnTopOrder = alwaysOnTopOrder;
		itemType->wareId = wareId;

		itemType->maxTextLen = maxTextLen;

		items.clientIdToServerId.emplace(itemType->clientId, itemType->id);

		if (itemType->id >= items.size())
		{
			size_t arbitrarySizeIncrease = 2000;
			size_t newSize = std::max<size_t>(static_cast<size_t>(itemType->id), items.size()) + arbitrarySizeIncrease;
			items.itemTypes.resize(newSize);
		}

		if (Appearances::hasObject(itemType->clientId))
		{
			auto &appearance = Appearances::getObjectById(itemType->clientId);

			// TODO: Check for items that do not have matching flags in .otb and appearances.dat
			itemType->blockSolid = hasBitSet(FLAG_BLOCK_SOLID, flags);
			itemType->blockProjectile = hasBitSet(FLAG_BLOCK_PROJECTILE, flags);
			itemType->blockPathFind = hasBitSet(FLAG_BLOCK_PATHFIND, flags);
			itemType->hasHeight = hasBitSet(FLAG_HAS_HEIGHT, flags);
			itemType->useable = hasBitSet(FLAG_USEABLE, flags) || appearance.hasFlag(AppearanceFlag::Usable);
			itemType->pickupable = hasBitSet(FLAG_PICKUPABLE, flags);
			itemType->moveable = hasBitSet(FLAG_MOVEABLE, flags);
			// itemType->stackable = hasBitSet(FLAG_STACKABLE, flags);
			itemType->stackable = appearance.hasFlag(AppearanceFlag::Cumulative);

			itemType->alwaysOnTop = hasBitSet(FLAG_ALWAYSONTOP, flags);
			itemType->isVertical = hasBitSet(FLAG_VERTICAL, flags);
			itemType->isHorizontal = hasBitSet(FLAG_HORIZONTAL, flags);
			itemType->isHangable = hasBitSet(FLAG_HANGABLE, flags);
			itemType->allowDistRead = hasBitSet(FLAG_ALLOWDISTREAD, flags);
			itemType->rotatable = hasBitSet(FLAG_ROTATABLE, flags);
			itemType->canReadText = hasBitSet(FLAG_READABLE, flags);
			itemType->lookThrough = hasBitSet(FLAG_LOOKTHROUGH, flags);
			itemType->isAnimation = hasBitSet(FLAG_ANIMATION, flags);
			// iType->walkStack = !hasBitSet(FLAG_FULLTILE, flags);
			itemType->forceUse = hasBitSet(FLAG_FORCEUSE, flags);

			itemType->appearance = &appearance;
			itemType->cacheTextureAtlases();
			if (appearance.name.size() != 0)
			{
				itemType->name = appearance.name;
			}
			if (itemType->name.size() == 0 && name.size() != 0)
			{
				itemType->name = std::move(name);
			}
		}

		[[maybe_unused]] uint8_t endToken = nextU8();
		DEBUG_ASSERT(endToken == EndNode, "Expected end token when parsing items.otb.");

	} while (!nodeEnd());
}

bool Items::OtbReader::nodeEnd() const
{
	bool endCursor = *cursor == EndNode;
	return endCursor;
}

ItemTypes_t Items::OtbReader::serverItemType(itemgroup_t group)
{
	switch (group)
	{
	case itemgroup_t::Container:
		return ItemTypes_t::Container;
	case itemgroup_t::Door:
		return ItemTypes_t::Door;
		break;
	case itemgroup_t::MagicField:
		//not used
		return ItemTypes_t::MagicField;
		break;
	case itemgroup_t::Teleport:
		//not used
		return ItemTypes_t::Teleport;
	case itemgroup_t::None:
	case itemgroup_t::Ground:
	case itemgroup_t::Splash:
	case itemgroup_t::Fluid:
	case itemgroup_t::Charges:
	case itemgroup_t::Deprecated:
		return ItemTypes_t::None;
	default:
		VME_LOG("Unknown item type!");
		return ItemTypes_t::None;
	}
}

uint8_t Items::OtbReader::peekByte()
{
	if (*cursor == EscapeNode)
	{
		return *(cursor + 1);
	}
	return *cursor;
}

uint8_t Items::OtbReader::nextU8()
{
	if (*cursor == EscapeNode)
	{
		++cursor;
	}

	uint8_t result = *cursor;
	++cursor;
	return result;
}

uint16_t Items::OtbReader::nextU16()
{
	uint16_t result = 0;
	int shift = 0;
	while (shift < 2)
	{
		if (*cursor == EscapeNode)
		{
			++cursor;
		}

		result += (*cursor) << (8 * shift);
		++cursor;
		++shift;
	}

	return result;
}

uint32_t Items::OtbReader::nextU32()
{
	uint16_t result = 0;
	int shift = 0;
	while (shift < 4)
	{
		if (*cursor == EscapeNode)
		{
			++cursor;
		}

		result += (*cursor) << (8 * shift);
		++cursor;
		++shift;
	}

	return result;
}

std::string Items::OtbReader::nextString(size_t length)
{
	std::string s;
	s.reserve(length);
	size_t current = 0;
	while (current < length)
	{
		if (*cursor == EscapeNode)
		{
			++cursor;
		}
		s.push_back(*cursor);
		++cursor;
		++current;
	}

	return s;
}

std::vector<uint8_t> Items::OtbReader::nextBytes(size_t bytes)
{
	std::vector<uint8_t> buffer(bytes);
	size_t current = 0;
	while (current < bytes)
	{
		if (*cursor == EscapeNode)
		{
			++cursor;
		}
		buffer[current] = *cursor;
		++cursor;
		++current;
	}

	return buffer;
}

void Items::OtbReader::skipBytes(size_t bytes)
{
	while (bytes > 0)
	{
		if (*cursor == EscapeNode)
		{
			++cursor;
		}
		++cursor;
		--bytes;
	}
}

ItemType *Items::getNextValidItemType(uint32_t serverId)
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

ItemType *Items::getPreviousValidItemType(uint32_t serverId)
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

const ItemType &Items::getItemIdByClientId(uint32_t clientId) const
{
	return itemTypes.at(clientIdToServerId.at(clientId));
}
