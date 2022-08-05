#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <pugixml.hpp>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "const.h"
#include "graphics/texture_atlas.h"
#include "item_type.h"
#include "observable_item.h"
#include "otb.h"
#include "position.h"
#include "signal.h"
#include "time_util.h"

/* Amount of cached texture atlas pointers that an ItemType can store.
  If the appearance of an ItemType has more than this amount of texture atlases,
  each lookup of the extra texture atlases will be O(log n), where n is the total
  amount of texture atlases.
*/

class Item;
enum class ItemTypes_t;

struct ItemSignal
{
    Nano::Signal<void(Item *)> address;
    Nano::Signal<void(ItemChangeType)> property;

    ItemSignal() {}
};

class Items
{
  public:
    static Items items;

    // Used to track (observe) items and be notified if the address of an item
    // with a given entity ID changes. The key is an entity ID.
    std::unordered_map<uint32_t, ItemSignal> itemSignals;

    // Used to track changes in a container (insert/remove item).
    // The key is an entity ID for the container item.
    std::unordered_map<uint32_t, Nano::Signal<void(ContainerChange)>> containerSignals;

    uint32_t highestServerId = 0;

    uint32_t createItemGid();
    void guidRefDestroyed(uint32_t id);
    void guidRefCreated(uint32_t id);

    static void loadFromOtb(const std::filesystem::path path);
    static void loadFromXml(const std::filesystem::path path);

    template <auto AddressFunction, auto PropertyFunction, typename T>
    ItemGuidDisconnect trackItem(uint32_t itemGuid, T *instance);

    template <auto MemberFunction, typename T>
    ItemGuidDisconnect trackContainer(uint32_t itemGuid, T *instance);

    void itemAddressChanged(Item *item);
    void itemPropertyChanged(Item *item, const ItemChangeType changeType);
    void containerChanged(Item *containerItem, const ContainerChange &containerChange);

    const std::vector<ItemType> &getItemTypes() const;

    /**
   * Load item types that are not present in the items.otb.
   */
    static void loadMissingItemTypes();

    bool reload();
    void clear();

    ItemType *getItemTypeByServerId(uint32_t serverId);
    bool validItemType(uint32_t serverId) const;
    ItemType *getItemTypeByClientId(uint32_t clientId);

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

    void addItemTypeAppearanceData(ItemType &itemType, uint32_t clientId, uint32_t itemTypeFlags);

    std::vector<ItemType> itemTypes;
    std::unordered_map<ClientID, ServerID> clientIdToServerId;
    // std::unordered_map<ServerID, MapID> serverIdToMapId;
    std::unordered_multimap<std::string, ServerID> nameToItems;

    OTB::VersionInfo _otbVersionInfo;

    uint32_t nextItemGuid = 0;
    std::queue<uint32_t> freedItemGuids;

    std::vector<uint16_t> guidRefCounts;
};

template <auto AddressFunction, auto PropertyFunction, typename T>
ItemGuidDisconnect Items::trackItem(uint32_t itemGuid, T *instance)
{
    auto outerFound = itemSignals.find(itemGuid);
    if (outerFound == itemSignals.end())
    {
        itemSignals.try_emplace(itemGuid);
        outerFound = itemSignals.find(itemGuid);
    }

    auto itemMoveSignals = &itemSignals;

    std::function<void()> disconnect = [itemMoveSignals, instance, itemGuid]() {
        auto found = itemMoveSignals->find(itemGuid);
        if (found != itemMoveSignals->end())
        {
            found->second.address.disconnect<AddressFunction>(instance);
            found->second.property.disconnect<PropertyFunction>(instance);
            if (found->second.address.is_empty() && found->second.property.is_empty())
            {
                itemMoveSignals->erase(itemGuid);
            }
        }
    };

    outerFound->second.address.connect<AddressFunction>(instance);
    outerFound->second.property.connect<PropertyFunction>(instance);

    return ItemGuidDisconnect(disconnect);
}

template <auto MemberFunction, typename T>
ItemGuidDisconnect Items::trackContainer(uint32_t itemGuid, T *instance)
{
    auto outerFound = containerSignals.find(itemGuid);
    if (outerFound == containerSignals.end())
    {
        containerSignals.try_emplace(itemGuid);
        outerFound = containerSignals.find(itemGuid);
    }

    auto containerMoveSignals = &containerSignals;

    std::function<void()> disconnect = [containerMoveSignals, instance, itemGuid]() {
        auto found = containerMoveSignals->find(itemGuid);
        if (found != containerMoveSignals->end())
        {
            found->second.disconnect<MemberFunction>(instance);
            if (found->second.is_empty())
            {
                containerMoveSignals->erase(itemGuid);
            }
        }
    };

    outerFound->second.connect<MemberFunction>(instance);

    return ItemGuidDisconnect(disconnect);
}

std::ostringstream stringify(const itemproperty_t &property);
std::ostream &operator<<(std::ostream &os, const itemproperty_t &property);