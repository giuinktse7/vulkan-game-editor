#pragma once

#include <array>

#include "const.h"
#include "graphics/appearances.h"
#include "graphics/texture_atlas.h"

#pragma warning(push)
#pragma warning(disable : 26812)

enum FloorChange
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

  const TextureInfo getTextureInfo(TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;
  const TextureInfo getTextureInfo(uint32_t spriteId, TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;
  const TextureInfo getTextureInfo(const Position &pos, TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;

  // For debugging purposes
  std::vector<TextureAtlas *> getTextureAtlases() const;

  TextureAtlas *getTextureAtlas(uint32_t spriteId) const;
  inline TextureAtlas *getFirstTextureAtlas() const noexcept;
  std::vector<const TextureAtlas *> atlases() const;

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
  inline bool isGroundBorder() const noexcept;
  inline bool hasFlag(AppearanceFlag flag) const noexcept;

  /*
    The items.otb may report a client ID that is incorrect. One such example
    is server ID 2812 that according to the items.otb has client ID 395.
    But there is no object with client ID 395.
    If the client ID is incorrect, it is set to 0.
  */
  inline bool isValid() const noexcept;
  inline bool hasAnimation() const noexcept;
  inline SpriteAnimation *animation() const noexcept;

  std::string getPluralName() const;

  inline int getElevation() const noexcept;
  bool hasElevation() const noexcept;

  std::string editorsuffix;
  std::string name;
  std::string article;
  std::string pluralName;
  std::string description;
  std::string runeSpellName;
  std::string vocationString;

  ItemType::Group group = ItemType::Group::None;
  ItemTypes_t type = ItemTypes_t::None;

  uint32_t weight = 0;
  uint32_t levelDoor = 0;
  uint32_t decayTime = 0;
  uint32_t wieldInfo = 0;
  uint32_t minReqLevel = 0;
  uint32_t minReqMagicLevel = 0;
  uint32_t charges = 0;
  int32_t maxHitChance = -1;
  int32_t decayTo = -1;
  int32_t attack = 0;
  int32_t defense = 0;
  int32_t extraDefense = 0;
  int32_t armor = 0;
  int32_t runeMagLevel = 0;
  int32_t runeLevel = 0;

  uint32_t id = 0;
  uint32_t clientId = 0;

  // CombatType_t combatType = COMBAT_NONE;
  uint16_t rotateTo = 0;
  uint16_t volume = 0;

  uint16_t transformToOnUse[2] = {0, 0};
  uint16_t transformToFree = 0;
  uint16_t destroyTo = 0;
  uint16_t maxTextLen = 0;
  uint16_t writeOnceItemId = 0;
  uint16_t transformEquipTo = 0;
  uint16_t transformDeEquipTo = 0;
  uint16_t maxItems = 8;
  uint16_t wareId = 0;

  MagicEffectClasses magicEffect = CONST_ME_NONE;
  GameDirection bedPartnerDir = DIRECTION_NONE;
  WeaponType_t weaponType = WEAPON_NONE;
  Ammo_t ammoType = AMMO_NONE;
  ShootType_t shootType = CONST_ANI_NONE;
  RaceType_t corpseType = RACE_NONE;
  FluidTypes_t fluidSource = FLUID_NONE;

  FloorChange floorChange = FloorChange::None;
  /*
    Also called alwaysOnTopOrder. Used to determine order for items on a tile,
    when more than one itemtype has alwaysBottomOfTile = true
  */
  uint8_t stackOrderIndex = 0;
  uint8_t lightLevel = 0;
  uint8_t lightColor = 0;
  uint8_t shootRange = 1;
  StackableSpriteType stackableSpriteType = StackableSpriteType::SingleId;
  int8_t hitChance = 0;

  bool forceUse = false;
  bool forceSerialize = false;
  bool hasHeight = false;
  bool blockSolid = false;
  bool blockPickupable = false;
  bool blockProjectile = false;
  bool blockPathFind = false;
  bool allowPickupable = false;
  bool showDuration = false;
  bool showCharges = false;
  bool showAttributes = false;
  bool replaceable = true;
  bool pickupable = false;
  bool rotatable = false;
  bool useable = false;
  bool moveable = false;
  bool alwaysBottomOfTile = false;
  bool canReadText = false;
  bool canWriteText = false;
  bool isVertical = false;
  bool isHorizontal = false;
  bool isHangable = false;
  bool allowDistRead = false;
  bool lookThrough = false;
  bool stopTime = false;
  bool showCount = true;
  bool decays = false;
  bool stackable = false;
  bool isAnimation = false;

  ObjectAppearance *appearance = nullptr;

  inline uint32_t speed() const noexcept;
  inline ItemSlot inventorySlot() const noexcept;

private:
  static constexpr size_t CachedTextureAtlasAmount = 5;

  std::array<TextureAtlas *, CachedTextureAtlasAmount> _atlases = {};

  void cacheTextureAtlas(uint32_t spriteId);
};

inline uint32_t ItemType::speed() const noexcept
{
  return isGroundTile() ? appearance->flagData.groundSpeed : 0;
}

inline ItemSlot ItemType::inventorySlot() const noexcept
{
  return appearance->flagData.itemSlot;
}

inline bool ItemType::hasFlag(AppearanceFlag flag) const noexcept
{
  return appearance->hasFlag(flag);
}

inline bool ItemType::isGroundBorder() const noexcept
{
  return hasFlag(AppearanceFlag::GroundBorder);
}

inline bool ItemType::isValid() const noexcept
{
  return clientId != 0;
}

inline bool ItemType::hasAnimation() const noexcept
{
  return appearance->getSpriteInfo().hasAnimation();
}

inline SpriteAnimation *ItemType::animation() const noexcept
{
  return appearance->getSpriteInfo().animation();
}

inline int ItemType::getElevation() const noexcept
{
  return appearance->flagData.elevation;
}

inline bool ItemType::hasElevation() const noexcept
{
  return appearance->hasFlag(AppearanceFlag::Height);
}

inline TextureAtlas *ItemType::getFirstTextureAtlas() const noexcept
{
  return _atlases.front();
}

#pragma warning(pop)
