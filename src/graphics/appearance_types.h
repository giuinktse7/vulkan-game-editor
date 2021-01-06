#pragma once

#include <stdint.h>

struct SpriteRange
{
    uint32_t start;
    uint32_t end;
};

enum class HookType
{
    None = -1,
    South = 1,
    East = 2
};

enum class AppearanceItemCategory
{
    Armors = 1,
    Amulets = 2,
    Boots = 3,
    Containers = 4,
    Decoration = 5,
    Food = 6,
    HelmetsHats = 7,
    Legs = 8,
    Others = 9,
    Potions = 10,
    Rings = 11,
    Runes = 12,
    Shields = 13,
    Tools = 14,
    Valuables = 15,
    Ammunition = 16,
    Axes = 17,
    Clubs = 18,
    DistanceWeapons = 19,
    Swords = 20,
    WandsRods = 21,
    PremiumScrolls = 22,
    TibiaCoins = 23,
    CreatureProducts = 24
};

enum class AppearancePlayerProfession
{
    Any = -1,
    None = 0,
    Knight = 1,
    Paladin = 2,
    Sorcerer = 3,
    Druid = 4,
    Promoted = 10
};

enum class AppearancePlayerDefaultAction
{
    None = 0,
    Look = 1,
    Use = 2,
    Open = 3,
    AutowalkHighlight = 4
};

/*
  These are values that represent how the client can interact with the appearance.
  They are mostly used for objects.
  See: https://tibia.fandom.com/wiki/Appearances.dat
*/
enum class AppearanceFlag : uint64_t
{
    /**
   * This is called 'Bank' in the protobuf file.
   */
    Ground = 1ULL << 0,
    /*
    If the appearance is ground but only partially covers it, for example the
    top tile where 2 different grounds are displayed.
    
    Corresponds to Clip in the protobuf Appearance data.
  */
    GroundBorder = 1ULL << 1,
    // If the appearance is the bottom-most appearance, only above the ground (bank).
    Bottom = 1ULL << 2,
    // If the appearance is the top-most appearance.
    Top = 1ULL << 3,
    // If the appearance is a container.
    Container = 1ULL << 4,
    // If the appearance is stackable.
    Cumulative = 1ULL << 5,
    // If the appearance has any use function.
    Usable = 1ULL << 6,
    Forceuse = 1ULL << 7,
    // If the appearance has any use with function.
    Multiuse = 1ULL << 8,
    // If the object is writable and editable.
    Write = 1ULL << 9,
    // If the object is writable, but only once.
    WriteOnce = 1ULL << 10,
    Liquidpool = 1ULL << 11,
    // If it's impossible to walk over the appearance.
    Unpass = 1ULL << 12,
    // If it's impossible to move the appearance.
    Unmove = 1ULL << 13,
    // If the appearance blocks the vision when trying to i.e. shoot projectiles or spells.
    Unsight = 1ULL << 14,
    // If the character will avoid walking over this object when automatically defining a walking path, even though it's walkable.
    Avoid = 1ULL << 15,
    NoMovementAnimation = 1ULL << 16,
    // If the object is pickupable, i.e. can be taken by players and stored into containers.
    Take = 1ULL << 17,
    // If the object is able to hold Liquids.
    LiquidContainer = 1ULL << 18,
    // If the object can be hanged on walls.
    Hang = 1ULL << 19,
    // The direction in which the object can be hanged: South or East.
    Hook = 1ULL << 20,
    // If the appearance can be rotated (such as many Furnitures).
    Rotate = 1ULL << 21,
    // If the appearance emits any kind of light. This flag is also used on some outfits, for example.
    Light = 1ULL << 22,
    DontHide = 1ULL << 23,
    Translucent = 1ULL << 24,
    // If the item should be offset when drawn. For example, a shift of x=5,y=10 offsets the item 5 pixels to the left and 10 pixels up from its original square (when rendered).
    Shift = 1ULL << 25,
    Height = 1ULL << 26,
    LyingObject = 1ULL << 27,
    AnimateAlways = 1ULL << 28,
    // If the appearance should be displayed on the automap (minimap).
    Automap = 1ULL << 29,
    // If the appearance should trigger the help icon when the help feature is used on the client.
    Lenshelp = 1ULL << 30,
    Fullbank = 1ULL << 31,
    // If the appearance must be ignored when the player looks it, meaning the appearance under it will be the one seen.
    IgnoreLook = 1ULL << 32,
    // If the object can be worn as an equipment.
    Clothes = 1ULL << 33,
    // The default action when using Left-Smart click mouse controls.
    DefaultAction = 1ULL << 34,
    Market = 1ULL << 35,
    // If the object can be into a furniture kit or similar.
    Wrap = 1ULL << 36,
    // If the object can be unwrapped.
    Unwrap = 1ULL << 37,
    Topeffect = 1ULL << 38,
    // NPC Buy and Sell data as displayed in the Cyclopedia.
    NpcSaleData = 1ULL << 39,
    // If the item can be switched on Action Bars using the Smart Switch option, currently available for Rings only.
    ChangedToExpire = 1ULL << 40,
    // If the object is a corpse.
    Corpse = 1ULL << 41,
    // If the object is a player's corpse.
    PlayerCorpse = 1ULL << 42,
    // If the object is in the Cyclopedia.
    CyclopediaItem = 1ULL << 43
};
