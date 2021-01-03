#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <pugixml.hpp>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "const.h"
#include "graphics/texture_atlas.h"
#include "item_type.h"
#include "otb.h"
#include "position.h"
#include "signal.h"
#include "time_point.h"
#include "tracked_item.h"

/* Amount of cached texture atlas pointers that an ItemType can store.
  If the appearance of an ItemType has more than this amount of texture atlases,
  each lookup of the extra texture atlases will be O(log n), where n is the total
  amount of texture atlases.
*/

class Item;
enum class ItemTypes_t;

class Items
{
public:
  static Items items;

  // Used to track (observe) items and be notified if the address of an item
  // with a given entity ID changes. The key is an entity ID.
  std::unordered_map<uint32_t, Nano::Signal<void(Item *)>> itemSignals;

  uint32_t highestClientId = 0;
  uint32_t highestServerId = 0;

  static void loadFromOtb(const std::filesystem::path path);
  static void loadFromXml(const std::filesystem::path path);

  template <auto MemberFunction, typename T>
  ItemEntityIdDisconnect trackItem(uint32_t entityId, T *instance);

  void itemMoved(Item *item);

  /**
   * Load item types that are not present in the items.otb.
   */
  static void loadMissingItemTypes();

  bool reload();
  void clear();

  ItemType *getItemTypeByServerId(uint32_t serverId);
  bool validItemType(uint32_t serverId) const;
  ItemType *getItemTypeByClientId(uint32_t clientId);

  const ItemType &getItemIdByClientId(uint32_t clientId) const;

  uint32_t getItemIdByName(const std::string &name);

  size_t size() const
  {
    return itemTypes.size();
  }

  ItemType *getNextValidItemType(uint32_t serverId);
  ItemType *getPreviousValidItemType(uint32_t serverId);

  const OTB::VersionInfo otbVersionInfo() const;

private:
  class OtbReader
  {
  public:
    OtbReader(const std::string &file);

    const uint8_t EscapeNode = 0xfd;
    const uint8_t StartNode = 0xfe;
    const uint8_t EndNode = 0xff;
    const uint8_t OTBM_ROOTV1 = 0x01;

    void parseOTB();

  private:
    TimePoint start;

    std::vector<uint8_t> buffer;
    OTB::ByteIterator cursor;
    std::string path;

    OTB::VersionInfo info;
    /*
    Something like 'OTB 3.62.78-11.1'.
  */
    std::string description;

    void readRoot();
    void readNodes();
    bool nodeEnd() const;

    uint8_t peekByte();
    uint8_t nextU8();
    uint16_t nextU16();
    uint32_t nextU32();
    std::vector<uint8_t> nextBytes(size_t bytes);
    std::string nextString(size_t length);
    void skipBytes(size_t bytes);

    ItemTypes_t serverItemType(ItemType::Group itemGroup);
  };

  Items();
  using ServerID = uint32_t;
  using ClientID = uint32_t;

  static bool loadItemFromXml(pugi::xml_node itemNode, uint32_t id);

  void addItemTypeAppearanceData(ItemType &itemType, uint32_t itemTypeFlags);

  std::vector<ItemType> itemTypes;
  std::unordered_map<ClientID, ServerID> clientIdToServerId;
  // std::unordered_map<ServerID, MapID> serverIdToMapId;
  std::unordered_multimap<std::string, ServerID> nameToItems;

  OTB::VersionInfo _otbVersionInfo;
};

template <auto MemberFunction, typename T>
ItemEntityIdDisconnect Items::trackItem(uint32_t entityId, T *instance)
{
  auto outerFound = itemSignals.find(entityId);
  if (outerFound == itemSignals.end())
  {
    itemSignals.try_emplace(entityId);
    outerFound = itemSignals.find(entityId);
  }

  auto itemMoveSignals = &itemSignals;

  std::function<void()> disconnect = [itemMoveSignals, instance, entityId]() {
    auto found = itemMoveSignals->find(entityId);
    if (found != itemMoveSignals->end())
    {
      found->second.disconnect<MemberFunction>(instance);
      if (found->second.is_empty())
      {
        itemMoveSignals->erase(entityId);
      }
    }
  };

  outerFound->second.connect<MemberFunction>(instance);

  return ItemEntityIdDisconnect(disconnect);
}

inline std::ostringstream stringify(const itemproperty_t &property)
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

inline std::ostream &operator<<(std::ostream &os, const itemproperty_t &property)
{
  os << stringify(property).str();
  return os;
}