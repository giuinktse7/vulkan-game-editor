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
#include "item.h"
#include "item_type.h"

Items Items::items;

namespace
{
    constexpr size_t InitialMaxGuids = static_cast<size_t>(util::power(2, 14));
    constexpr uint32_t ReservedItemCount = 45000;

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
        // Fluids in current version (?), might change in the future
        else if (50000 < id && id < 50100)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    static std::unordered_map<std::string, FloorChange> const FloorChangeFromStringTable = {
        {"down", FloorChange::Down},
        {"down", FloorChange::Down},
        {"north", FloorChange::North},
        {"south", FloorChange::South},
        {"west", FloorChange::West},
        {"east", FloorChange::East},
        {"northex", FloorChange::NorthEx},
        {"southex", FloorChange::SouthEx},
        {"westex", FloorChange::WestEx},
        {"eastex", FloorChange::EastEx},
        {"southalt", FloorChange::SouthAlt},
        {"eastalt", FloorChange::EastAlt}};
} // namespace

Items::Items()
    : guidRefCounts(InitialMaxGuids)
{
}

uint32_t Items::createItemGid()
{
    uint32_t id;
    if (freedItemGuids.empty())
    {
        id = nextItemGuid++;
    }
    else
    {
        id = freedItemGuids.front();
        freedItemGuids.pop();
    }

    if (guidRefCounts.size() <= id)
    {
        guidRefCounts.resize(guidRefCounts.size() * 4);
    }

    guidRefCounts.at(id) = 1;

    return id;
}

void Items::guidRefCreated(uint32_t id)
{
    uint16_t &refCount = guidRefCounts.at(id);
    DEBUG_ASSERT(refCount != 0, "There is no item with that uid.");
    ++refCount;
}

void Items::guidRefDestroyed(uint32_t id)
{
    // Happens for example when a creature with an item look is destructured
    if (id >= guidRefCounts.size())
    {
        return;
    }

    uint16_t &refCount = guidRefCounts.at(id);
    --refCount;
    if (refCount == 0)
    {
        freedItemGuids.emplace(id);
    }
}

ItemType *Items::getItemTypeByServerId(uint32_t serverId)
{
    if (serverId >= itemTypes.size())
        return nullptr;

    return &itemTypes.at(serverId);
}

bool Items::validItemType(uint32_t serverId) const
{
    if (serverId >= itemTypes.size())
        return false;

    return itemTypes.at(serverId).appearance != nullptr;
}

ItemType *Items::getItemTypeByClientId(uint32_t clientId)
{
    if (!clientIdToServerId.contains(clientId))
        return nullptr;

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

void Items::itemAddressChanged(Item *item)
{
    auto found = itemSignals.find(item->guid());
    if (found != itemSignals.end())
    {
        found->second.address.fire(item);
    }
}

void Items::itemPropertyChanged(Item *item, const ItemChangeType changeType)
{
    auto found = itemSignals.find(item->guid());
    if (found != itemSignals.end())
    {
        found->second.property.fire(changeType);
    }
}

void Items::containerChanged(Item *containerItem, const ContainerChange &containerChange)
{
    auto found = containerSignals.find(containerItem->guid());
    if (found != containerSignals.end())
    {
        found->second.fire(containerChange);
    }
}

bool Items::loadItemFromXml(pugi::xml_node itemNode, uint32_t id)
{
    if (!Items::items.validItemType(id) || reservedForFluid(id, DefaultVersion))
        return false;

    ItemType &it = *Items::items.getItemTypeByServerId(id);

    it.setName(itemNode.attribute("name").as_string());
    // it.editorsuffix = itemNode.attribute("editorsuffix").as_string();

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
                it.group = ItemType::Group::MagicField;
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
                it.setName(attribute.as_string());
            }
        }
        else if (key == "description")
        {
            if ((attribute = itemAttributesNode.attribute("value")))
            {
                it.description = attribute.as_string();
            }
        }
        // Currently no use for runeSpellName
        // else if (key == "runespellName")
        // {
        /*if((attribute = itemAttributesNode.attribute("value"))) {
				it.runeSpellName = attribute.as_string();
			}*/
        // }
        else if (key == "weight")
        {
            if ((attribute = itemAttributesNode.attribute("value")))
            {
                it.weight = static_cast<uint32_t>(std::floor(attribute.as_int() / 100.f));
            }
        }
        // Currenly no use for armor (2021-01-13)
        // else if (key == "armor")
        // {
        // if ((attribute = itemAttributesNode.attribute("value")))
        // {
        //     it.armor = attribute.as_int();
        // }
        // }
        // Currenly no use for defense (2021-01-13)
        // else if (key == "defense")
        // {

        // if ((attribute = itemAttributesNode.attribute("value")))
        // {
        //     it.defense = attribute.as_int();
        // }
        // }
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
        // else if (key == "decayto")
        // {
        //     it.decays = true;
        // }
        else if (key == "maxtextlen" || key == "maxtextlength")
        {
            if ((attribute = itemAttributesNode.attribute("value")))
            {
                it.maxTextLen = attribute.as_int();
                it.canReadText = it.maxTextLen > 0;
            }
        }
        // Currenly no use for writeonceitemid (2021-01-13)
        // else if (key == "writeonceitemid")
        // {
        /*if((attribute = itemAttributesNode.attribute("value"))) {
				it.writeOnceItemId = pugi::cast<int32_t>(attribute.value());
			}*/
        // }
        // Currenly no use for allowdistread (2021-01-13)
        // else if (key == "allowdistread")
        // {
        // if ((attribute = itemAttributesNode.attribute("value")))
        // {
        //     it.allowDistRead = attribute.as_bool();
        // }
        // }
        else if (key == "charges")
        {
            if ((attribute = itemAttributesNode.attribute("value")))
            {
                // Ignore (maybe should not?)
            }
        }
        else if (key == "floorchange")
        {
            if ((attribute = itemAttributesNode.attribute("value")))
            {
                std::string floorChangeString = attribute.as_string();

                auto found = FloorChangeFromStringTable.find(floorChangeString);
                if (found != FloorChangeFromStringTable.end())
                {
                    it.floorChange = found->second;
                }
                else
                {
                    VME_LOG_ERROR("Unknown floorchange: " << floorChangeString);
                }
            }
        }
    }
    return true;
}

void Items::addItemTypeAppearanceData(ItemType &itemType, uint32_t clientId, uint32_t flags)
{
    auto &appearance = Appearances::getObjectById(clientId);

    if (appearance.hasFlag(AppearanceFlag::Ground))
    {
        itemType.group = ItemType::Group::Ground;
        itemType.stackOrder = TileStackOrder::Ground;
    }
    else if (appearance.hasFlag(AppearanceFlag::Border))
    {
        itemType.stackOrder = TileStackOrder::Border;
    }
    else if (appearance.hasFlag(AppearanceFlag::Bottom))
    {
        itemType.stackOrder = TileStackOrder::Bottom;
    }
    else if (appearance.hasFlag(AppearanceFlag::Top))
    {
        itemType.stackOrder = TileStackOrder::Top;
    }
    else if (appearance.hasFlag(AppearanceFlag::Container))
    {
        itemType.group = ItemType::Group::Container;
    }

    // TODO: Check for items that do not have matching flags in .otb and appearances.dat
    // itemType.blockSolid = hasBitSet(FLAG_BLOCK_SOLID, flags);
    // itemType.blockProjectile = hasBitSet(FLAG_BLOCK_PROJECTILE, flags) || appearance.hasFlag(AppearanceFlag::Unsight);
    // itemType.blockPathFind = hasBitSet(FLAG_BLOCK_PATHFIND, flags);
    // itemType.useable = hasBitSet(FLAG_USEABLE, flags) || appearance.hasFlag(AppearanceFlag::Usable);
    itemType.pickupable = hasBitSet(FLAG_PICKUPABLE, flags) || appearance.hasFlag(AppearanceFlag::Take);
    // itemType.moveable = hasBitSet(FLAG_MOVEABLE, flags) || !appearance.hasFlag(AppearanceFlag::Unmove);
    // itemType.stackable = hasBitSet(FLAG_STACKABLE, flags);
    itemType.stackable = appearance.hasFlag(AppearanceFlag::Cumulative);
    itemType.maxTextLen = std::max(
        static_cast<uint32_t>(itemType.maxTextLen),
        std::max(appearance.flagData.maxTextLength, appearance.flagData.maxTextLengthOnce));

    itemType.isVertical = hasBitSet(FLAG_VERTICAL, flags);
    itemType.isHorizontal = hasBitSet(FLAG_HORIZONTAL, flags);
    itemType.isHangable = hasBitSet(FLAG_HANGABLE, flags) || appearance.hasFlag(AppearanceFlag::Hang);
    // itemType.allowDistRead = hasBitSet(FLAG_ALLOWDISTREAD, flags);
    // itemType.rotatable = hasBitSet(FLAG_ROTATABLE, flags) || appearance.hasFlag(AppearanceFlag::Rotate);
    itemType.canReadText = hasBitSet(FLAG_READABLE, flags) || appearance.hasFlag(AppearanceFlag::Write) || appearance.hasFlag(AppearanceFlag::WriteOnce);
    itemType.lookThrough = hasBitSet(FLAG_LOOKTHROUGH, flags);
    // itemType.isAnimation = hasBitSet(FLAG_ANIMATION, flags);
    // itemType.forceUse = hasBitSet(FLAG_FORCEUSE, flags) || appearance.hasFlag(AppearanceFlag::Forceuse);

    itemType.appearance = &appearance;
    itemType.cacheTextureAtlases();
}

void Items::loadMissingItemTypes()
{
    Items &items = Items::items;
    std::vector<uint32_t> clientIds;
    for (const auto &pair : Appearances::objects())
    {
        clientIds.emplace_back(pair.first);
    }

    std::sort(clientIds.begin(), clientIds.end());

    for (const auto clientId : clientIds)
    {
        if (items.clientIdToServerId.find(clientId) == items.clientIdToServerId.end())
        {
            ++items.highestServerId;
            uint32_t serverId = items.highestServerId;

            // VME_LOG("Loading object id " << clientId << " as server id " << serverId);

            if (serverId >= items.size())
            {
                size_t arbitrarySizeIncrease = 2000;
                size_t newSize = std::max<size_t>(static_cast<size_t>(serverId), items.size()) + arbitrarySizeIncrease;
                items.itemTypes.resize(newSize);
            }

            ItemType &itemType = items.itemTypes.at(serverId);
            itemType.id = serverId;

            items.clientIdToServerId.emplace(clientId, serverId);

            items.addItemTypeAppearanceData(itemType, clientId, 0);
        }
    }
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

    nextU8();  // First byte of otb is zero
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
    info.buildNumber = nextU32();  // build number, revision
    Items::items._otbVersionInfo = info;

    // OTB description, something like 'OTB 3.62.78-11.1'
    std::vector<uint8_t> buffer = util::sliceLeading<uint8_t>(nextBytes(128), 0);
    description = std::string(buffer.begin(), buffer.end());

    readNodes();

    Items::items.itemTypes.shrink_to_fit();
    std::ostringstream otbInfo;
    otbInfo << info.majorVersion << "." << info.minorVersion << ", build: " << info.buildNumber;
    VME_LOG("Loaded items.otb (" << otbInfo.str() << ") in " << this->start.elapsedMillis() << " ms.");
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
    // items.itemTypes.reserve(ReservedItemCount);
    items.clientIdToServerId.reserve(ReservedItemCount);
    items.nameToItems.reserve(ReservedItemCount);

    do
    {
        [[maybe_unused]] uint8_t startNode = nextU8();
        DEBUG_ASSERT(startNode == StartNode, "startNode must be OtbReader::StartNode.");

        // ItemType itemType;

        uint8_t itemGroup = nextU8();
        uint32_t flags = nextU32();

        uint32_t serverId = 0;
        uint32_t clientId = 0;
        // uint16_t speed = 0;
        uint16_t lightLevel = 0;
        uint16_t lightColor = 0;
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

                    items.highestServerId = std::max(items.highestServerId, serverId);

                    // if (serverId >= items.size())
                    // {
                    //     items.itemTypes.resize(static_cast<size_t>(serverId) + 1);
                    // }

                    if (serverId >= items.size())
                    {
                        size_t arbitrarySizeIncrease = 2000;
                        size_t newSize = std::max<size_t>(static_cast<size_t>(serverId), items.size()) + arbitrarySizeIncrease;
                        items.itemTypes.resize(newSize);
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
                    // Speed is added through the appearance protobuf file so we can just
                    // ignore it here.
                    DEBUG_ASSERT(attributeSize == sizeof(uint16_t), "Invalid attribute length.");
                    skipBytes(attributeSize);
                    // speed = nextU16();
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
                    // This information is gotten from the protobuf file instead.
                    // AppearanceFlag::Ground => Ground item (corresponds to toporder 0)
                    // AppearanceFlag::Border => Border item (corresponds to toporder 1)
                    // AppearanceFlag::Bottom => Bottom item (corresponds to toporder 2)
                    // AppearanceFlag::Top => Top item (corresponds to toporder 3)

                    DEBUG_ASSERT(attributeSize == sizeof(uint8_t), "Invalid attribute length.");
                    skipBytes(attributeSize);
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
                    name = nextString(attributeSize);
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

        items.clientIdToServerId.emplace(clientId, serverId);

        itemType->id = serverId;

        if (Appearances::hasObject(clientId))
        {
            items.addItemTypeAppearanceData(*itemType, clientId, flags);

            switch (itemType->id)
            {
                case 8133:
                    // Earth ground 64x64, earth on top-left and bottom-right quadrants
                    itemType->appearance->quadrantRenderType = QuadrantRenderType::TopLeftBottomRight;
                    break;
                case 919:
                    // Mountain ground 64x64, mountain on top-left quadrant
                    itemType->appearance->quadrantRenderType = QuadrantRenderType::TopLeft;
                    break;
                case 877:
                    // Mountain bottom-right wall piece 64x64, mountain wall on every quadrant except top-left
                    itemType->appearance->quadrantRenderType = QuadrantRenderType::TopRightBottomRightBottomLeft;
                    break;
                default:
                    break;
            }

            itemType->group = static_cast<ItemType::Group>(itemGroup);
            itemType->type = serverItemType(itemType->group);

            // itemType->speed = speed;
            itemType->appearance->flagData.brightness = static_cast<uint8_t>(lightLevel);
            itemType->appearance->flagData.color = static_cast<uint8_t>(lightColor);

            itemType->wareId = wareId;
            itemType->maxTextLen = maxTextLen;

            if (itemType->name().size() == 0 && name.size() != 0)
            {
                itemType->setName(std::move(name));
            }
        }

        [[maybe_unused]] uint8_t endToken = nextU8();
        DEBUG_ASSERT(endToken == EndNode, "Expected end token when parsing items.otb.");

    } while (!nodeEnd());
}

const std::vector<ItemType> &Items::getItemTypes() const
{
    return itemTypes;
}

ItemTypes_t Items::OtbReader::serverItemType(ItemType::Group itemGroup)
{
    switch (itemGroup)
    {
        case ItemType::Group::Container:
            return ItemTypes_t::Container;
        case ItemType::Group::Door:
            return ItemTypes_t::Door;
            break;
        case ItemType::Group::MagicField:
            //not used
            return ItemTypes_t::MagicField;
            break;
        case ItemType::Group::Teleport:
            //not used
            return ItemTypes_t::Teleport;
        case ItemType::Group::None:
        case ItemType::Group::Ground:
        case ItemType::Group::Splash:
        case ItemType::Group::Fluid:
        case ItemType::Group::Charges:
        case ItemType::Group::Deprecated:
            return ItemTypes_t::None;
        default:
            VME_LOG("Unknown item type!");
            return ItemTypes_t::None;
    }
}

bool Items::OtbReader::nodeEnd() const
{
    return *cursor == EndNode;
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
    uint32_t result = 0;
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

const OTB::VersionInfo Items::otbVersionInfo() const
{
    return _otbVersionInfo;
}

std::ostringstream stringify(const itemproperty_t &property)
{
    std::ostringstream s;

    switch (property)
    {
        case ITEM_ATTR_FIRST:
            s << "ITEM_ATTR_SERVERID";
            break;
        case ITEM_ATTR_CLIENTID:
            s << "ITEM_ATTR_CLIENTID";
            break;
        case ITEM_ATTR_NAME:
            s << "ITEM_ATTR_NAME";
            break;
        case ITEM_ATTR_DESCR:
            s << "ITEM_ATTR_DESCR";
            break;
        case ITEM_ATTR_SPEED:
            s << "ITEM_ATTR_SPEED";
            break;
        case ITEM_ATTR_SLOT:
            s << "ITEM_ATTR_SLOT";
            break;
        case ITEM_ATTR_MAXITEMS:
            s << "ITEM_ATTR_MAXITEMS";
            break;
        case ITEM_ATTR_WEIGHT:
            s << "ITEM_ATTR_WEIGHT";
            break;
        case ITEM_ATTR_WEAPON:
            s << "ITEM_ATTR_WEAPON";
            break;
        case ITEM_ATTR_AMU:
            s << "ITEM_ATTR_AMU";
            break;
        case ITEM_ATTR_ARMOR:
            s << "ITEM_ATTR_ARMOR";
            break;
        case ITEM_ATTR_MAGLEVEL:
            s << "ITEM_ATTR_MAGLEVEL";
            break;
        case ITEM_ATTR_MAGFIELDTYPE:
            s << "ITEM_ATTR_MAGFIELDTYPE";
            break;
        case ITEM_ATTR_WRITEABLE:
            s << "ITEM_ATTR_WRITEABLE";
            break;
        case ITEM_ATTR_ROTATETO:
            s << "ITEM_ATTR_ROTATETO";
            break;
        case ITEM_ATTR_DECAY:
            s << "ITEM_ATTR_DECAY";
            break;
        case ITEM_ATTR_SPRITEHASH:
            s << "ITEM_ATTR_SPRITEHASH";
            break;
        case ITEM_ATTR_MINIMAPCOLOR:
            s << "ITEM_ATTR_MINIMAPCOLOR";
            break;
        case ITEM_ATTR_MAX_TEXT_LENGTH:
            s << "ITEM_ATTR_MAX_TEXT_LENGTH";
            break;
        case ITEM_ATTR_MAX_TEXT_LENGTH_ONCE:
            s << "ITEM_ATTR_MAX_TEXT_LENGTH_ONCE";
            break;
        case ITEM_ATTR_LIGHT:
            s << "ITEM_ATTR_LIGHT";
            break;
        case ITEM_ATTR_DECAY2:
            s << "ITEM_ATTR_DECAY2";
            break;
        case ITEM_ATTR_WEAPON2:
            s << "ITEM_ATTR_WEAPON2";
            break;
        case ITEM_ATTR_AMU2:
            s << "ITEM_ATTR_AMU2";
            break;
        case ITEM_ATTR_ARMOR2:
            s << "ITEM_ATTR_ARMOR2";
            break;
        case ITEM_ATTR_WRITEABLE2:
            s << "ITEM_ATTR_WRITEABLE2";
            break;
        case ITEM_ATTR_LIGHT2:
            s << "ITEM_ATTR_LIGHT2";
            break;
        case ITEM_ATTR_TOPORDER:
            s << "ITEM_ATTR_TOPORDER";
            break;
        case ITEM_ATTR_WRITEABLE3:
            s << "ITEM_ATTR_WRITEABLE3";
            break;
        case ITEM_ATTR_WAREID:
            s << "ITEM_ATTR_WAREID";
            break;
        case ITEM_ATTR_LAST:
            s << "ITEM_ATTR_LAST";
            break;
        default:
            s << "Unknown itemproperty_t: " << static_cast<int>(to_underlying(property));
            break;
    }

    return s;
}

std::ostream &operator<<(std::ostream &os, const itemproperty_t &property)
{
    os << stringify(property).str();
    return os;
}