#pragma once

#include <array>
#include <tuple>
#include <utility>

#include "graphics/appearance_types.h"
#include "graphics/texture_atlas.h"
#include "position.h"
#include "sprite_info.h"

#pragma warning(push)
#pragma warning(disable : 26812)

class ObjectAppearance;
struct SpriteAnimation;
class RawBrush;

enum class ItemDataType
{
    Normal,
    Teleport,
    HouseDoor,
    Depot,
    Container
};

enum FloorChange : uint8_t
{
    Down,
    West,
    East,
    North,
    South,
    WestEx,
    EastEx,
    NorthEx,
    SouthEx,
    SouthAlt,
    EastAlt,
    None
};

enum itemproperty_t : uint8_t
{
    ITEM_ATTR_FIRST = 0x10,
    ITEM_ATTR_SERVERID = ITEM_ATTR_FIRST,
    ITEM_ATTR_CLIENTID,
    ITEM_ATTR_NAME,
    ITEM_ATTR_DESCR,
    ITEM_ATTR_SPEED,
    ITEM_ATTR_SLOT,
    ITEM_ATTR_MAXITEMS,
    ITEM_ATTR_WEIGHT,
    ITEM_ATTR_WEAPON,
    ITEM_ATTR_AMU,
    ITEM_ATTR_ARMOR,
    ITEM_ATTR_MAGLEVEL,
    ITEM_ATTR_MAGFIELDTYPE,
    ITEM_ATTR_WRITEABLE,
    ITEM_ATTR_ROTATETO,
    ITEM_ATTR_DECAY,
    ITEM_ATTR_SPRITEHASH,
    ITEM_ATTR_MINIMAPCOLOR,
    /*
    Can be rewritten.
  */
    ITEM_ATTR_MAX_TEXT_LENGTH,
    /*
    Can only be written once.
  */
    ITEM_ATTR_MAX_TEXT_LENGTH_ONCE,
    ITEM_ATTR_LIGHT,

    //1-byte aligned
    ITEM_ATTR_DECAY2,     //deprecated
    ITEM_ATTR_WEAPON2,    //deprecated
    ITEM_ATTR_AMU2,       //deprecated
    ITEM_ATTR_ARMOR2,     //deprecated
    ITEM_ATTR_WRITEABLE2, //deprecated
    ITEM_ATTR_LIGHT2,
    ITEM_ATTR_TOPORDER,
    ITEM_ATTR_WRITEABLE3, //deprecated

    ITEM_ATTR_WAREID,

    ITEM_ATTR_LAST
};

enum class ItemTypes_t
{
    None,
    Depot,
    Mailbox,
    TrashHolder,
    Container,
    Door,
    MagicField,
    Teleport,
    Bed,
    Key,
    Rune,
    Last
};

enum ItemFlags_t
{
    FLAG_BLOCK_SOLID = 1 << 0,
    FLAG_BLOCK_PROJECTILE = 1 << 1,
    FLAG_BLOCK_PATHFIND = 1 << 2,
    FLAG_HAS_HEIGHT = 1 << 3,
    FLAG_USEABLE = 1 << 4,
    FLAG_PICKUPABLE = 1 << 5,
    FLAG_MOVEABLE = 1 << 6,
    FLAG_STACKABLE = 1 << 7,
    FLAG_FLOORCHANGEDOWN = 1 << 8,   // unused
    FLAG_FLOORCHANGENORTH = 1 << 9,  // unused
    FLAG_FLOORCHANGEEAST = 1 << 10,  // unused
    FLAG_FLOORCHANGESOUTH = 1 << 11, // unused
    FLAG_FLOORCHANGEWEST = 1 << 12,  // unused
    FLAG_ALWAYSONTOP = 1 << 13,
    FLAG_READABLE = 1 << 14,
    FLAG_ROTATABLE = 1 << 15,
    FLAG_HANGABLE = 1 << 16,
    FLAG_VERTICAL = 1 << 17,
    FLAG_HORIZONTAL = 1 << 18,
    FLAG_CANNOTDECAY = 1 << 19, // unused
    FLAG_ALLOWDISTREAD = 1 << 20,
    FLAG_UNUSED = 1 << 21,        // unused
    FLAG_CLIENTCHARGES = 1 << 22, /* deprecated */
    FLAG_LOOKTHROUGH = 1 << 23,
    FLAG_ANIMATION = 1 << 24,
    FLAG_FULLTILE = 1 << 25, // unused
    FLAG_FORCEUSE = 1 << 26,
};

/**
 * A stackable ItemType can have either one sprite ID for all counts, or eight
 * sprite IDs for different counts.
 * Note: Often, several of the eight sprite ids are the same, like for
 * wood (serverId: 5901), which is of the form X,Y,Z,Z,Z,Z,Z,Z where
 * X, Y and Z are different sprite IDs.
 */
enum class StackableSpriteType : uint8_t
{
    SingleId,
    EightIds
};

enum class StackSizeOffset
{
    One = 0,
    Two = 1,
    Three = 2,
    Four = 3,
    Five = 4,
    Ten = 5,
    TwentyFive = 6,
    Fifty = 7
};

class ItemType
{
  public:
    enum class Group
    {
        None,

        Ground,
        Container,
        Weapon,     //deprecated
        Ammunition, //deprecated
        Armor,      //deprecated
        Charges,
        Teleport,   //deprecated
        MagicField, //deprecated
        Writable,   //deprecated
        Key,        //deprecated
        Splash,
        Fluid,
        Door, //deprecated
        Deprecated,

        ITEM_GROUP_LAST
    };

    ItemType() {}
    //non-copyable
    ItemType(const ItemType &other) = delete;
    ItemType &operator=(const ItemType &other) = delete;

    ItemType(ItemType &&) = default;
    ItemType &operator=(ItemType &&) = default;

    void cacheTextureAtlases();

    const uint32_t getPatternIndex(const Position &pos) const;
    const uint32_t getPatternIndexForSubtype(uint8_t subtype) const;

    uint32_t getSpriteId(const Position &pos) const;

    const TextureInfo getTextureInfo(TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;
    const TextureInfo getTextureInfo(uint32_t spriteId, TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;
    const TextureInfo getTextureInfo(const Position &pos, TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;
    const TextureInfo getTextureInfoForSubtype(uint8_t subtype, TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;

    const TextureInfo getTextureInfoTopLeftQuadrant(uint32_t spriteId) const;
    const std::pair<TextureInfo, TextureInfo> getTextureInfoTopLeftBottomRightQuadrant(uint32_t spriteId) const;
    const std::tuple<TextureInfo, TextureInfo, TextureInfo> getTextureInfoTopRightBottomRightBottomLeftQuadrant(uint32_t spriteId) const;

    const SpriteInfo &getSpriteInfo(size_t frameGroup) const;
    const SpriteInfo &getSpriteInfo() const;

    // For debugging purposes
    std::vector<TextureAtlas *> getTextureAtlases() const;

    TextureAtlas *getTextureAtlas(uint32_t spriteId) const;
    inline TextureAtlas *getFirstTextureAtlas() const noexcept;
    std::vector<const TextureAtlas *> atlases() const;

    [[nodiscard]] const std::string &name() const noexcept;
    [[nodiscard]] uint32_t clientId() const noexcept;

    bool isGroundTile() const noexcept;
    bool isContainer() const noexcept;
    bool isSplash() const noexcept;
    bool isFluidContainer() const noexcept;
    bool isCorpse() const noexcept;

    bool isDoor() const noexcept;
    bool isMagicField() const noexcept;
    bool isTeleport() const noexcept;
    bool isKey() const noexcept;
    bool isDepot() const noexcept;
    bool isMailbox() const noexcept;
    bool isTrashHolder() const noexcept;
    bool isBed() const noexcept;

    bool isRune() const noexcept;
    bool isPickupable() const noexcept;
    bool isUseable() const noexcept;
    bool hasSubType() const noexcept;

    bool usesSubType() const noexcept;
    bool isStackable() const noexcept;
    bool isGroundBorder() const noexcept;
    bool hasFlag(AppearanceFlag flag) const noexcept;

    /*
    The items.otb may report a client ID that is incorrect. One such example
    is server ID 2812 that according to the items.otb has client ID 395.
    But there is no object with client ID 395.
    If the client ID is incorrect, it is set to 0.
  */
    inline bool isValid() const noexcept;
    bool hasAnimation() const noexcept;
    SpriteAnimation *animation() const noexcept;

    std::string getPluralName() const;

    int getElevation() const noexcept;
    bool hasElevation() const noexcept;

    uint32_t speed() const noexcept;
    ItemSlot inventorySlot() const noexcept;

    [[nodiscard]] bool alwaysBottomOfTile() const noexcept;

    // std::string editorsuffix;
    std::string article;
    std::string pluralName;
    std::string description;
    // std::string runeSpellName;

    ObjectAppearance *appearance = nullptr;
    RawBrush *rawBrush = nullptr;

    ItemType::Group group = ItemType::Group::None;
    ItemTypes_t type = ItemTypes_t::None;

    uint32_t id = 0;
    uint32_t weight = 0;
    uint32_t levelDoor = 0;
    uint32_t charges = 0;

    // CombatType_t combatType = COMBAT_NONE;
    uint16_t rotateTo = 0;
    uint16_t volume = 0;

    uint16_t maxTextLen = 0;
    uint16_t writeOnceItemId = 0;
    uint16_t maxItems = 0;
    uint16_t wareId = 0;

    FloorChange floorChange = FloorChange::None;
    /*
        Also called alwaysOnTopOrder. Used to determine order for items on a tile,
        when more than one itemtype has alwaysBottomOfTile = true
    */
    uint8_t stackOrderIndex = 0;
    StackableSpriteType stackableSpriteType = StackableSpriteType::SingleId;

    // bool blockSolid = false;
    // bool blockProjectile = false;
    // bool blockPathFind = false;
    bool allowPickupable = false;
    // bool showDuration = false;
    // bool showCharges = false;
    // bool showAttributes = false;
    // bool replaceable = true;
    bool pickupable = false;
    // bool rotatable = false;
    // bool useable = false;
    // bool moveable = false;
    // bool alwaysBottomOfTile = false;
    bool canReadText = false;
    bool canWriteText = false;
    bool isVertical = false;
    bool isHorizontal = false;
    bool isHangable = false;
    // bool allowDistRead = false;
    bool lookThrough = false;
    // bool stopTime = false;
    bool showCount = true;
    bool stackable = false;

  private:
    static constexpr size_t CachedTextureAtlasAmount = 5;

    std::array<TextureAtlas *, CachedTextureAtlasAmount> _atlases = {};

    void cacheTextureAtlas(uint32_t spriteId);
};

inline bool ItemType::isGroundBorder() const noexcept
{
    return hasFlag(AppearanceFlag::GroundBorder);
}

inline bool ItemType::isValid() const noexcept
{
    return appearance != nullptr;
}

inline TextureAtlas *ItemType::getFirstTextureAtlas() const noexcept
{
    return _atlases.front();
}

#pragma warning(pop)
