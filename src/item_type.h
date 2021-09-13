#pragma once

#include <array>
#include <tuple>
#include <utility>
#include <vector>

#include "frame_group.h"
#include "graphics/appearance_types.h"
#include "graphics/texture_atlas.h"
#include "position.h"
#include "sprite_info.h"

#pragma warning(push)
#pragma warning(disable : 26812)

/**
 * Flags for fast property checks
 */
enum class ItemTypeFlag
{
    None = 0,
    InGroundBrush = 1,
    InBorderBrush = 1 << 1,
    InMountainBrush = 1 << 2,
    InWallBrush = 1 << 3
};
VME_ENUM_OPERATORS(ItemTypeFlag);

class ObjectAppearance;
struct SpriteAnimation;
class Brush;
class BorderBrush;
class GroundBrush;
enum class BrushType : int;
class RawBrush;

enum class ItemDataType
{
    Normal,
    Teleport,
    HouseDoor,
    Depot,
    Container
};

enum class FloorChange : uint8_t
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

    // Called FLAG_ALWAYSONTOP in other software (like Remere's Map Editor)
    FLAG_DIRECTLY_ABOVE_BORDER = 1 << 13,
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

enum class FluidColor : uint8_t
{
    Transparent = 0,
    Blue,
    Red,
    Brown,
    Green,
    Yellow,
    White,
    Purple
};

enum class FluidType : uint8_t
{
    None = 0,
    Water = static_cast<uint8_t>(FluidColor::Blue),
    Blood = static_cast<uint8_t>(FluidColor::Red),
    Beer = static_cast<uint8_t>(FluidColor::Brown),
    Slime = static_cast<uint8_t>(FluidColor::Green),
    Lemonade = static_cast<uint8_t>(FluidColor::Yellow),
    Milk = static_cast<uint8_t>(FluidColor::White),
    Mana = static_cast<uint8_t>(FluidColor::Purple),
    Life = static_cast<uint8_t>(FluidColor::Red) + 8,
    Oil = static_cast<uint8_t>(FluidColor::Brown) + 8,
    Urine = static_cast<uint8_t>(FluidColor::Yellow) + 8,
    CoconutMilk = static_cast<uint8_t>(FluidColor::White) + 8,
    Wine = static_cast<uint8_t>(FluidColor::Purple) + 8,

    Mud = static_cast<uint8_t>(FluidColor::Brown) + 16,
    FruitJuice = static_cast<uint8_t>(FluidColor::Yellow) + 16,
    Ink = static_cast<uint8_t>(FluidColor::White) + 16, //12.20+ - we don't care about this id so let's choose whatever

    Lava = static_cast<uint8_t>(FluidColor::Red) + 24,
    Rum = static_cast<uint8_t>(FluidColor::Brown) + 24,
    Swamp = static_cast<uint8_t>(FluidColor::Green) + 24,

    Tea = static_cast<uint8_t>(FluidColor::Brown) + 32,

    Mead = static_cast<uint8_t>(FluidColor::Brown) + 40,
};

inline FluidType fluidTypeFromIndex(int index)
{
    switch (index)
    {
        case 0:
            return FluidType::None;
        case 1:
            return FluidType::Water;
        case 2:
            return FluidType::Blood;
        case 3:
            return FluidType::Beer;
        case 4:
            return FluidType::Slime;
        case 5:
            return FluidType::Lemonade;
        case 6:
            return FluidType::Milk;
        case 7:
            return FluidType::Mana;
        case 8:
            return FluidType::Life;
        case 9:
            return FluidType::Oil;
        case 10:
            return FluidType::Urine;
        case 11:
            return FluidType::CoconutMilk;
        case 12:
            return FluidType::Wine;
        case 13:
            return FluidType::Mud;
        case 14:
            return FluidType::FruitJuice;
        case 15:
            return FluidType::Ink;
        case 16:
            return FluidType::Lava;
        case 17:
            return FluidType::Rum;
        case 18:
            return FluidType::Swamp;
        case 19:
            return FluidType::Tea;
        case 20:
            return FluidType::Mead;
        default:
        {
            VME_LOG_ERROR("Could not convert" << index << " to fluid type. Using FluidType::None instead.");
            return FluidType::None;
        }
    }
}

inline uint8_t indexOfFluidType(FluidType fluidType)
{
    switch (fluidType)
    {
        case FluidType::None:
            return 0;
        case FluidType::Water:
            return 1;
        case FluidType::Blood:
            return 2;
        case FluidType::Beer:
            return 3;
        case FluidType::Slime:
            return 4;
        case FluidType::Lemonade:
            return 5;
        case FluidType::Milk:
            return 6;
        case FluidType::Mana:
            return 7;
        case FluidType::Life:
            return 8;
        case FluidType::Oil:
            return 9;
        case FluidType::Urine:
            return 10;
        case FluidType::CoconutMilk:
            return 11;
        case FluidType::Wine:
            return 12;
        case FluidType::Mud:
            return 13;
        case FluidType::FruitJuice:
            return 14;
        case FluidType::Ink:
            return 15;
        case FluidType::Lava:
            return 16;
        case FluidType::Rum:
            return 17;
        case FluidType::Swamp:
            return 18;
        case FluidType::Tea:
            return 19;
        case FluidType::Mead:
            return 20;
        default:
            return 0;
    }
}

enum class TileStackOrder : uint8_t
{
    Ground = 0,
    Border = 1,
    // Things like trees, walls, stones, statues, pillars, ...
    Bottom = 2,

    // Items with TileStackOrder `Default` are placed on top of all other items, except for `Top` items.
    Default = 3,

    // Things like archways, open doors, open fence gates, ...
    Top = UINT8_MAX
};

// [[nodiscard]] TileStackOrder TileStackOrderFromValue(uint8_t value, bool *ok = nullptr) noexcept;

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

    const std::vector<FrameGroup> &frameGroups() const noexcept;

    // For debugging purposes
    std::vector<TextureAtlas *> getTextureAtlases() const;

    TextureAtlas *getTextureAtlas(uint32_t spriteId) const;
    TextureAtlas *getFirstTextureAtlas() const noexcept;
    std::vector<const TextureAtlas *> atlases() const;

    [[nodiscard]] const std::string &name() const noexcept;
    [[nodiscard]] uint32_t clientId() const noexcept;

    void setName(std::string name) noexcept;

    bool isGround() const noexcept;
    bool isContainer() const noexcept;
    bool isSplash() const noexcept;
    bool isChargeable() const noexcept;
    bool isFluidContainer() const noexcept;
    bool isCorpse() const noexcept;
    bool isWriteable() const noexcept;
    bool isBottom() const noexcept;

    bool isDoor() const noexcept;
    bool isMagicField() const noexcept;
    bool isTeleport() const noexcept;
    bool isKey() const noexcept;
    bool isDepot() const noexcept;
    bool isMailbox() const noexcept;
    bool isTrashHolder() const noexcept;
    bool isBed() const noexcept;
    bool isBlocking() const noexcept;

    bool isRune() const noexcept;
    bool isPickupable() const noexcept;
    bool isUseable() const noexcept;
    bool hasSubType() const noexcept;

    bool usesSubType() const noexcept;
    bool isStackable() const noexcept;
    bool isBorder() const noexcept;
    bool hasFlag(AppearanceFlag flag) const noexcept;
    inline bool hasFlag(ItemTypeFlag flag) const noexcept;

    Brush *brush() const noexcept;

    Brush *getBrush(BrushType type) const noexcept;

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

    void setFlag(ItemTypeFlag flag) noexcept;

    void addBrush(Brush *brush);

    // std::string editorsuffix;
    std::string article;
    std::string pluralName;
    std::string description;
    // std::string runeSpellName;

    ObjectAppearance *appearance = nullptr;

    ItemType::Group group = ItemType::Group::None;
    ItemTypes_t type = ItemTypes_t::None;

    uint32_t id = 0;
    uint32_t weight = 0;
    uint32_t levelDoor = 0;

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
    TileStackOrder stackOrder = TileStackOrder::Default;

    StackableSpriteType stackableSpriteType = StackableSpriteType::SingleId;

    bool allowPickupable = false;
    bool pickupable = false;
    bool canReadText = false;
    bool canWriteText = false;
    bool isVertical = false;
    bool isHorizontal = false;
    bool isHangable = false;
    bool lookThrough = false;
    bool showCount = true;
    bool stackable = false;

    ItemTypeFlag flags = ItemTypeFlag::None;

  private:
    uint8_t getFluidPatternOffset(FluidType fluidType) const;

    // Ground/border/wall brush
    GroundBrush *groundBrush = nullptr;
    BorderBrush *borderBrush = nullptr;
    Brush *otherBrush = nullptr;

    std::string _name;
};

inline bool ItemType::isBorder() const noexcept
{
    return hasFlag(AppearanceFlag::Border) || hasFlag(ItemTypeFlag::InBorderBrush);
}

inline bool ItemType::isValid() const noexcept
{
    return appearance != nullptr;
}

inline bool ItemType::hasFlag(ItemTypeFlag flag) const noexcept
{
    return to_underlying(flags & flag);
}

#pragma warning(pop)
