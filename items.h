#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>
#include <memory>
#include <pugixml.hpp>
#include <vector>
#include <set>
#include <array>

#include "otb.h"
#include "const.h"

#include "position.h"
#include "graphics/texture_atlas.h"
#include "graphics/appearances.h"

#include "item_type.h"

/* Amount of cached texture atlas pointers that an ItemType can store.
  If the appearance of an ItemType has more than this amount of texture atlases,
  each lookup of the extra texture atlases will be O(log n), where n is the total
  amount of texture atlases.
*/

class Items
{
public:
  static void loadFromOtb(const std::string &file);

  static void loadFromXml(const std::filesystem::path path);

  bool reload();
  void clear();

  const bool hasItemType(size_t id) const
  {
    return itemTypes.size() > id;
  }

  ItemType *getItemType(uint16_t id);
  ItemType *getItemTypeByClientId(uint16_t clientId);

  const ItemType &getItemIdByClientId(uint16_t spriteId) const;

  uint16_t getItemIdByName(const std::string &name);

  size_t size() const
  {
    return itemTypes.size();
  }

  static Items items;

  ItemType *getNextValidItemType(uint16_t serverId);
  ItemType *getPreviousValidItemType(uint16_t serverId);

  OTB::VersionInfo getOtbVersionInfo();

private:
  Items();
  using ServerID = uint16_t;
  using ClientID = uint16_t;
  // MapID is used to filter out ItemTypes that do not have an appearance (e.g. invalid ItemTypes).
  // using MapID = uint16_t;

  static bool loadItemFromXml(pugi::xml_node itemNode, uint32_t id);

  std::vector<ItemType> itemTypes;
  std::unordered_map<ClientID, ServerID> clientIdToServerId;
  // std::unordered_map<ServerID, MapID> serverIdToMapId;
  std::unordered_multimap<std::string, ServerID> nameToItems;

  OTB::VersionInfo otbVersionInfo;

  /* Used to hide invalid item ids */
  // TODO Neither are used right now
  std::set<uint16_t> validItemTypeStartId;
  std::set<uint16_t> validItemTypeEndId;
};