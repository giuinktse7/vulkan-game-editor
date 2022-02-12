#pragma once

#include <stdint.h>

#include "debug.h"
#include "util.h"

// Filesystem constants
constexpr char APP_FOLDER_PATH[] = ".vme";
constexpr char RECENT_FILES_FILE[] = "recent_files.json";

// Map constants
constexpr uint16_t MAP_TREE_CHILDREN_COUNT = 16;
constexpr uint16_t MAP_LAYERS = 16;
constexpr int GROUND_FLOOR = 7;

constexpr int MapTileSize = 32;
constexpr int SpriteSize = 32;

enum class WindowType
{
    Normal,
    Hatch
};

enum class DoorType
{
    Normal,
    Locked,
    Quest,
    Magic
};

enum class WallType
{
    Horizontal,
    Vertical,
    Corner,
    Pole
};

enum class TileQuadrant
{
    TopLeft = 1,
    TopRight = 1 << 1,
    BottomRight = 1 << 2,
    BottomLeft = 1 << 3
};
VME_ENUM_OPERATORS(TileQuadrant)

enum class BorderType
{
    None = 0,
    North = 1,
    East = 2,
    South = 3,
    West = 4,
    NorthWestCorner = 5,
    NorthEastCorner = 6,
    SouthEastCorner = 7,
    SouthWestCorner = 8,
    NorthWestDiagonal = 9,
    NorthEastDiagonal = 10,
    SouthEastDiagonal = 11,
    SouthWestDiagonal = 12,
    Center = 13
};

enum class BorderExpandDirection
{
    North,
    East,
    South,
    West,
    None
};

enum class Direction
{
    North = 0,
    East = 1,
    South = 2,
    West = 3
};

enum class ItemSlot : uint8_t
{
    TwoHanded = 0,
    Head = 1,
    Amulet = 2,
    Container = 3,
    Torso = 4,
    Shield = 5,
    OneHanded = 6,
    Legs = 7,
    Feet = 8,
    Ring = 9,
    // Ammunition slot
    Belt = 10,
    StoreInbox = 11,
    ShoppingBag = 12,
    None = UINT8_MAX
};

enum class SpriteLayout
{
    ONE_BY_ONE = 0,
    ONE_BY_TWO = 1,
    TWO_BY_ONE = 2,
    TWO_BY_TWO = 3
};

enum MagicEffectClasses : uint8_t
{
    CONST_ME_NONE,

    CONST_ME_DRAWBLOOD = 1,
    CONST_ME_LOSEENERGY = 2,
    CONST_ME_POFF = 3,
    CONST_ME_BLOCKHIT = 4,
    CONST_ME_EXPLOSIONAREA = 5,
    CONST_ME_EXPLOSIONHIT = 6,
    CONST_ME_FIREAREA = 7,
    CONST_ME_YELLOW_RINGS = 8,
    CONST_ME_GREEN_RINGS = 9,
    CONST_ME_HITAREA = 10,
    CONST_ME_TELEPORT = 11,
    CONST_ME_ENERGYHIT = 12,
    CONST_ME_MAGIC_BLUE = 13,
    CONST_ME_MAGIC_RED = 14,
    CONST_ME_MAGIC_GREEN = 15,
    CONST_ME_HITBYFIRE = 16,
    CONST_ME_HITBYPOISON = 17,
    CONST_ME_MORTAREA = 18,
    CONST_ME_SOUND_GREEN = 19,
    CONST_ME_SOUND_RED = 20,
    CONST_ME_POISONAREA = 21,
    CONST_ME_SOUND_YELLOW = 22,
    CONST_ME_SOUND_PURPLE = 23,
    CONST_ME_SOUND_BLUE = 24,
    CONST_ME_SOUND_WHITE = 25,
    CONST_ME_BUBBLES = 26,
    CONST_ME_CRAPS = 27,
    CONST_ME_GIFT_WRAPS = 28,
    CONST_ME_FIREWORK_YELLOW = 29,
    CONST_ME_FIREWORK_RED = 30,
    CONST_ME_FIREWORK_BLUE = 31,
    CONST_ME_STUN = 32,
    CONST_ME_SLEEP = 33,
    CONST_ME_WATERCREATURE = 34,
    CONST_ME_GROUNDSHAKER = 35,
    CONST_ME_HEARTS = 36,
    CONST_ME_FIREATTACK = 37,
    CONST_ME_ENERGYAREA = 38,
    CONST_ME_SMALLCLOUDS = 39,
    CONST_ME_HOLYDAMAGE = 40,
    CONST_ME_BIGCLOUDS = 41,
    CONST_ME_ICEAREA = 42,
    CONST_ME_ICETORNADO = 43,
    CONST_ME_ICEATTACK = 44,
    CONST_ME_STONES = 45,
    CONST_ME_SMALLPLANTS = 46,
    CONST_ME_CARNIPHILA = 47,
    CONST_ME_PURPLEENERGY = 48,
    CONST_ME_YELLOWENERGY = 49,
    CONST_ME_HOLYAREA = 50,
    CONST_ME_BIGPLANTS = 51,
    CONST_ME_CAKE = 52,
    CONST_ME_GIANTICE = 53,
    CONST_ME_WATERSPLASH = 54,
    CONST_ME_PLANTATTACK = 55,
    CONST_ME_TUTORIALARROW = 56,
    CONST_ME_TUTORIALSQUARE = 57,
    CONST_ME_MIRRORHORIZONTAL = 58,
    CONST_ME_MIRRORVERTICAL = 59,
    CONST_ME_SKULLHORIZONTAL = 60,
    CONST_ME_SKULLVERTICAL = 61,
    CONST_ME_ASSASSIN = 62,
    CONST_ME_STEPSHORIZONTAL = 63,
    CONST_ME_BLOODYSTEPS = 64,
    CONST_ME_STEPSVERTICAL = 65,
    CONST_ME_YALAHARIGHOST = 66,
    CONST_ME_BATS = 67,
    CONST_ME_SMOKE = 68,
    CONST_ME_INSECTS = 69,
    CONST_ME_DRAGONHEAD = 70,
    CONST_ME_ORCSHAMAN = 71,
    CONST_ME_ORCSHAMAN_FIRE = 72,
    CONST_ME_THUNDER = 73,
    CONST_ME_FERUMBRAS = 74,
    CONST_ME_CONFETTI_HORIZONTAL = 75,
    CONST_ME_CONFETTI_VERTICAL = 76,
    // 77-157 are empty
    CONST_ME_BLACKSMOKE = 158,
    // 159-166 are empty
    CONST_ME_REDSMOKE = 167,
    CONST_ME_YELLOWSMOKE = 168,
    CONST_ME_GREENSMOKE = 169,
    CONST_ME_PURPLESMOKE = 170,
    CONST_ME_EARLY_THUNDER = 171,
    CONST_ME_RAGIAZ_BONECAPSULE = 172,
    CONST_ME_CRITICAL_DAMAGE = 173,
    // 174 is empty
    CONST_ME_PLUNGING_FISH = 175,
};

enum WeaponType_t : uint8_t
{
    WEAPON_NONE,
    WEAPON_SWORD,
    WEAPON_CLUB,
    WEAPON_AXE,
    WEAPON_SHIELD,
    WEAPON_DISTANCE,
    WEAPON_WAND,
    WEAPON_AMMO,
};

enum Ammo_t : uint8_t
{
    AMMO_NONE,
    AMMO_BOLT,
    AMMO_ARROW,
    AMMO_SPEAR,
    AMMO_THROWINGSTAR,
    AMMO_THROWINGKNIFE,
    AMMO_STONE,
    AMMO_SNOWBALL,
};

enum WeaponAction_t : uint8_t
{
    WEAPONACTION_NONE,
    WEAPONACTION_REMOVECOUNT,
    WEAPONACTION_REMOVECHARGE,
    WEAPONACTION_MOVE,
};

enum WieldInfo_t
{
    WIELDINFO_NONE = 0 << 0,
    WIELDINFO_LEVEL = 1 << 0,
    WIELDINFO_MAGLV = 1 << 1,
    WIELDINFO_VOCREQ = 1 << 2,
    WIELDINFO_PREMIUM = 1 << 3,
};

enum Skulls_t : uint8_t
{
    SKULL_NONE = 0,
    SKULL_YELLOW = 1,
    SKULL_GREEN = 2,
    SKULL_WHITE = 3,
    SKULL_RED = 4,
    SKULL_BLACK = 5,
    SKULL_ORANGE = 6,
};

enum ShootType_t : uint8_t
{
    CONST_ANI_NONE,

    CONST_ANI_SPEAR = 1,
    CONST_ANI_BOLT = 2,
    CONST_ANI_ARROW = 3,
    CONST_ANI_FIRE = 4,
    CONST_ANI_ENERGY = 5,
    CONST_ANI_POISONARROW = 6,
    CONST_ANI_BURSTARROW = 7,
    CONST_ANI_THROWINGSTAR = 8,
    CONST_ANI_THROWINGKNIFE = 9,
    CONST_ANI_SMALLSTONE = 10,
    CONST_ANI_DEATH = 11,
    CONST_ANI_LARGEROCK = 12,
    CONST_ANI_SNOWBALL = 13,
    CONST_ANI_POWERBOLT = 14,
    CONST_ANI_POISON = 15,
    CONST_ANI_INFERNALBOLT = 16,
    CONST_ANI_HUNTINGSPEAR = 17,
    CONST_ANI_ENCHANTEDSPEAR = 18,
    CONST_ANI_REDSTAR = 19,
    CONST_ANI_GREENSTAR = 20,
    CONST_ANI_ROYALSPEAR = 21,
    CONST_ANI_SNIPERARROW = 22,
    CONST_ANI_ONYXARROW = 23,
    CONST_ANI_PIERCINGBOLT = 24,
    CONST_ANI_WHIRLWINDSWORD = 25,
    CONST_ANI_WHIRLWINDAXE = 26,
    CONST_ANI_WHIRLWINDCLUB = 27,
    CONST_ANI_ETHEREALSPEAR = 28,
    CONST_ANI_ICE = 29,
    CONST_ANI_EARTH = 30,
    CONST_ANI_HOLY = 31,
    CONST_ANI_SUDDENDEATH = 32,
    CONST_ANI_FLASHARROW = 33,
    CONST_ANI_FLAMMINGARROW = 34,
    CONST_ANI_SHIVERARROW = 35,
    CONST_ANI_ENERGYBALL = 36,
    CONST_ANI_SMALLICE = 37,
    CONST_ANI_SMALLHOLY = 38,
    CONST_ANI_SMALLEARTH = 39,
    CONST_ANI_EARTHARROW = 40,
    CONST_ANI_EXPLOSION = 41,
    CONST_ANI_CAKE = 42,

    CONST_ANI_TARSALARROW = 44,
    CONST_ANI_VORTEXBOLT = 45,

    CONST_ANI_PRISMATICBOLT = 48,
    CONST_ANI_CRYSTALLINEARROW = 49,
    CONST_ANI_DRILLBOLT = 50,
    CONST_ANI_ENVENOMEDARROW = 51,

    CONST_ANI_GLOOTHSPEAR = 53,
    CONST_ANI_SIMPLEARROW = 54,

    // for internal use, don't send to client
    CONST_ANI_WEAPONTYPE = 0xFE, // 254
};

enum RaceType_t : uint8_t
{
    RACE_NONE,
    RACE_VENOM,
    RACE_BLOOD,
    RACE_UNDEAD,
    RACE_FIRE,
    RACE_ENERGY,
};