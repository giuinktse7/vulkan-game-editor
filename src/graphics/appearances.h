#pragma once

#include <unordered_map>
#include <memory>
#include <filesystem>
#include <array>

#include <sstream>
#include <variant>

#ifndef MESSAGES_WRAPPER_H
#define MESSAGES_WRAPPER_H

#pragma warning(push, 0)
#include "protobuf/appearances.pb.h"
#pragma warning(pop)

#endif // MESSAGES_WRAPPER_H

#include "../../vendor/tsl/robin_map.h"

#include "../debug.h"
#include "../position.h"
#include "../const.h"

#include "texture_atlas.h"

#define default_unordered_map tsl::robin_map

namespace proto
{
  using namespace tibia::protobuf;
  using Appearance = appearances::Appearance;
  using Appearances = appearances::Appearances;
  using AppearanceFlags = appearances::AppearanceFlags;
  using SpriteInfo = appearances::SpriteInfo;
  using SpriteAnimation = appearances::SpriteAnimation;

  using ANIMATION_LOOP_TYPE = shared::ANIMATION_LOOP_TYPE;
  using HOOK_TYPE = shared::HOOK_TYPE;
  using PLAYER_ACTION = shared::PLAYER_ACTION;
  using ITEM_CATEGORY = shared::ITEM_CATEGORY;
} // namespace proto

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
*/
enum class AppearanceFlag : uint64_t
{
  Bank = 1ULL << 0,
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

enum class AnimationLoopType
{
  PingPong = -1,
  Infinite = 0,
  Counted = 1
};

inline AppearanceFlag operator|(AppearanceFlag a, AppearanceFlag b)
{
  return static_cast<AppearanceFlag>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline AppearanceFlag operator&(AppearanceFlag a, AppearanceFlag b)
{
  return static_cast<AppearanceFlag>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}

inline AppearanceFlag operator&=(AppearanceFlag a, AppearanceFlag b)
{
  return (AppearanceFlag &)((uint64_t &)a &= (uint64_t)b);
}

inline AppearanceFlag &operator|=(AppearanceFlag &a, AppearanceFlag b)
{
  return (AppearanceFlag &)((uint64_t &)a |= (uint64_t)b);
}

inline bool operator>(AppearanceFlag a, uint64_t b)
{
  return (uint64_t)a > b;
}

struct SpritePhase
{
  uint32_t minDuration;
  uint32_t maxDuration;
};

enum FixedFrameGroup
{
  OutfitIdle = 0,
  OutfitMoving = 1,
  ObjectInitial = 2
};

struct SpriteAnimation
{
  uint32_t defaultStartPhase;
  bool synchronized;
  /*
    If true, the animation can start in any phase.
  */
  bool randomStartPhase;
  AnimationLoopType loopType;

  // Amount of times to loop. Only relevant if the loopType is AnimationLoopType::Counted.
  uint32_t loopCount;
  std::vector<SpritePhase> phases;

  friend struct SpriteInfo;

private:
  static SpriteAnimation fromProtobufData(proto::SpriteAnimation animation);
};

struct SpriteInfo
{
  uint32_t patternWidth = 0;
  uint32_t patternHeight = 0;
  uint32_t patternDepth = 0;
  uint32_t layers = 1;

  std::vector<uint32_t> spriteIds;

  uint32_t boundingSquare;
  bool isOpaque;

  bool hasAnimation() const
  {
    return _animation != nullptr;
  }

  SpriteAnimation *animation() const;

  friend class Appearance;

  // repeated Box bounding_box_per_direction = 9;
private:
  static SpriteInfo fromProtobufData(proto::SpriteInfo info);

  std::unique_ptr<SpriteAnimation> _animation;
};

struct FrameGroup
{
  FixedFrameGroup fixedGroup;
  uint32_t id;
  SpriteInfo spriteInfo;
};

/*
  Convenience wrapper for a tibia::protobuf Appearance
*/
class Appearance
{
public:
  Appearance(proto::Appearance protobufAppearance);
  Appearance(const Appearance &) = delete;

  Appearance(Appearance &&other) noexcept;
  Appearance &operator=(Appearance &&) = default;

  const uint32_t getSpriteId(uint32_t frameGroup, int index) const;
  const uint32_t getSpriteId(int index) const;
  uint32_t getSpriteId(uint32_t frameGroup, Position pos);
  uint32_t getFirstSpriteId() const;
  const SpriteInfo &getSpriteInfo(size_t frameGroup) const;
  const SpriteInfo &getSpriteInfo() const;

  bool hasFlag(AppearanceFlag flag)
  {
    return (flags & flag) > 0ULL;
  }

  size_t frameGroupCount() const;

  uint32_t clientId;
  std::string name;

  // Appearance flag data
  struct AppearanceFlagData
  {
    uint32_t bankWaypoints;
    uint32_t maxTextLength;
    uint32_t maxTextLengthOnce;
    // Represents the radius of the emitted light.
    uint32_t brightness;
    uint32_t color;
    int elevation = 0;
    uint32_t shiftX = 0;
    uint32_t shiftY = 0;
    uint32_t itemSlot;
    AppearancePlayerDefaultAction defaultAction = AppearancePlayerDefaultAction::None;
    struct
    {
      AppearanceItemCategory category;
      uint32_t tradeAsId;
      uint32_t showAsId;
      AppearancePlayerProfession restrictedProfession;
      uint32_t minLevel;
    } market;
    // The automap (minimap) color.
    uint32_t automapColor;
    HookType hookDirection = HookType::None;
    uint32_t lenshelp;
    uint32_t changedToExpireFormerObjectTypeId;
    uint32_t cyclopediaClientId;
  } flagData = {};

private:
  // Most (if not all) objects have only one FrameGroup. This avoids having to create a vector to store a single element (perf not tested as of 2020/08/02).
  std::variant<SpriteInfo, std::vector<FrameGroup>> appearanceData;
  AppearanceFlag flags;
};

class Appearances
{
  using AppearanceId = uint32_t;

public:
  static void loadTextureAtlases(const std::filesystem::path catalogContentsPath);
  static void loadAppearanceData(const std::filesystem::path path);

  static bool hasObject(AppearanceId id)
  {
    return objects.find(id) != objects.end();
  }

  static Appearance &getObjectById(AppearanceId id)
  {
    return objects.at(id);
  }

  static TextureAtlas *getTextureAtlas(const uint32_t spriteId);

  static bool isLoaded;

  static size_t textureAtlasCount();

private:
  static default_unordered_map<AppearanceId, Appearance> objects;
  static default_unordered_map<AppearanceId, proto::Appearance> outfits;

  /* 
		Used for quick retrieval of the correct spritesheet given a sprite ID.
		It stores the upper bound of the sprite ids in the sprite sheet.
	*/
  static std::set<uint32_t> catalogIndex;

  static default_unordered_map<uint32_t, std::unique_ptr<TextureAtlas>> textureAtlases;

  /* 
		Used for quick retrieval of a texture atlas given a sprite ID.
		It stores the start and end sprite id in the sprite sheet.
	*/
  static std::vector<SpriteRange> textureAtlasSpriteRanges;
};

inline std::ostream &operator<<(std::ostream &os, const HookType &hookType)
{
  std::ostringstream s;

  switch (hookType)
  {
  case HookType::East:
    s << "East";
    break;
  case HookType::South:
    s << "South";
    break;
  case HookType::None:
    s << "None";
    break;
  default:
    s << "Unknown HookType";
    break;
  }

  os << s.str();
  return os;
}

inline std::ostringstream stringify(const AppearancePlayerDefaultAction &action)
{
  std::ostringstream s;

  switch (action)
  {
  case AppearancePlayerDefaultAction::AutowalkHighlight:
    s << "AutowalkHighlight";
    break;
  case AppearancePlayerDefaultAction::Look:
    s << "Look";
    break;
  case AppearancePlayerDefaultAction::None:
    s << "None";
    break;
  case AppearancePlayerDefaultAction::Open:
    s << "Open";
    break;
  case AppearancePlayerDefaultAction::Use:
    s << "Use";
    break;
  default:
    s << "Unknown AppearancePlayerDefaultAction";
    break;
  }
  return s;
}

inline std::ostream &operator<<(std::ostream &os, const AppearancePlayerDefaultAction &action)
{
  os << stringify(action).str();
  return os;
}

inline std::ostringstream stringify(const Appearance::AppearanceFlagData &flags)
{
  std::ostringstream s;

  s << "{\n";

#define WRITE_INT(flag)                                      \
  do                                                         \
  {                                                          \
    if (flags.flag != 0)                                     \
    {                                                        \
      s << "\t" << #flag << ": " << flags.flag << std::endl; \
    }                                                        \
  } while (false)

  WRITE_INT(bankWaypoints);
  WRITE_INT(maxTextLength);
  WRITE_INT(maxTextLengthOnce);
  WRITE_INT(brightness);
  WRITE_INT(color);
  WRITE_INT(shiftX);
  WRITE_INT(shiftY);
  WRITE_INT(itemSlot);
  WRITE_INT(market.tradeAsId);
  WRITE_INT(market.showAsId);
  WRITE_INT(market.minLevel);
  WRITE_INT(automapColor);
  WRITE_INT(lenshelp);
  WRITE_INT(changedToExpireFormerObjectTypeId);
  WRITE_INT(cyclopediaClientId);
  if (flags.defaultAction != AppearancePlayerDefaultAction::None)
  {
    s << "\tdefaultAction: " << flags.defaultAction << std::endl;
  }
  if (flags.hookDirection != HookType::None)
  {
    s << "\thookDirection: " << flags.hookDirection << std::endl;
  }

  s << "}" << std::endl;

  return s;
}

inline std::ostream &operator<<(std::ostream &os, const Appearance::AppearanceFlagData &flags)
{
  os << stringify(flags).str();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const Appearance &appearance)
{
  std::ostringstream s;

  s << "{\n";
  s << "\tclientId: " << appearance.clientId << std::endl;
  s << "\tflags: " << appearance.flagData << std::endl;

  os << s.str();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const proto::SpriteInfo &info)
{
  std::ostringstream s;

  s << "{ ";
  if (info.has_pattern_width())
    s << "pattern_width=" << info.pattern_width() << ", ";

  if (info.has_pattern_height())
    s << "pattern_height=" << info.pattern_height() << ", ";

  if (info.pattern_depth())
    s << "pattern_depth=" << info.pattern_depth() << ", ";

  if (info.has_layers())
    s << "layers=" << info.layers() << ", ";

  s << "sprite_ids=";

  for (const auto id : info.sprite_id())
    s << id << ", ";

  if (info.has_bounding_square())
    s << "bounding_square=" << info.bounding_square() << ", ";

  if (info.has_animation())
    s << "has_animation= true, ";

  if (info.has_is_opaque())
    s << "is_opaque = " << info.is_opaque() << ", ";

  // Print bounding box information
  s << "bboxes: " << info.bounding_box_per_direction_size() << std::endl;
  // for (const auto bbox : info.bounding_box_per_direction())
  //   s << std::endl
  //     << "\t{ x=" << bbox.x() << ", y=" << bbox.y() << ", width=" << bbox.width() << ", height=" << bbox.height() << "}";

  os << s.str();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const proto::AppearanceFlags &flags)
{
  std::ostringstream s;
  s << std::endl;

  // #define PRINT_FLAG_UTIL(a)                                \
//   do                                                      \
//   {                                                       \
//     if (flags.has_##a() && flags.##a())                   \
//     {                                                     \
//       s << "\t" << #a << ":" << flags.##a() << std::endl; \
//     }                                                     \
//   } while (false)

  if (flags.has_bank())
  {
    std::cout << "bank.waypoints: " << flags.bank().waypoints() << std::endl;
  }
  // PRINT_FLAG_UTIL(clip);
  // PRINT_FLAG_UTIL(bottom);
  // PRINT_FLAG_UTIL(top);
  // PRINT_FLAG_UTIL(container);
  // PRINT_FLAG_UTIL(cumulative);
  // PRINT_FLAG_UTIL(usable);
  // PRINT_FLAG_UTIL(forceuse);
  // PRINT_FLAG_UTIL(multiuse);
  // PRINT_FLAG_UTIL(liquidpool);
  // PRINT_FLAG_UTIL(unpass);
  // PRINT_FLAG_UTIL(unmove);
  // PRINT_FLAG_UTIL(unsight);
  // PRINT_FLAG_UTIL(avoid);
  // PRINT_FLAG_UTIL(no_movement_animation);
  // PRINT_FLAG_UTIL(take);
  // PRINT_FLAG_UTIL(liquidcontainer);
  // PRINT_FLAG_UTIL(hang);
  if (flags.has_hook())
  {
    if (flags.hook().has_direction())
      s << "\thook: " << flags.hook().direction() << std::endl;
    else
      s << "\thook (no direction)" << std::endl;
  }

  // PRINT_FLAG_UTIL(rotate);
  if (flags.has_light())
  {
    s << "\tlight: ("
      << "brightness: " << flags.light().brightness() << ", color: " << flags.light().color() << ")" << std::endl;
  }
  // PRINT_FLAG_UTIL(dont_hide);
  // PRINT_FLAG_UTIL(translucent);
  if (flags.has_shift())
  {
    s << "\tshift: (";
    if (flags.shift().has_x())
    {
      s << "x=" << flags.shift().x() << " ";
    }
    if (flags.shift().has_y())
    {
      s << "y=" << flags.shift().y();
    }

    s << ")" << std::endl;
  }

  if (flags.has_height())
  {
    if (flags.height().has_elevation())
    {
      s << "\theight elevation: " << flags.height().elevation() << std::endl;
    }
    else
    {
      s << "\theight (no elevation)" << std::endl;
    }
  }
  // PRINT_FLAG_UTIL(lying_object);
  // PRINT_FLAG_UTIL(animate_always);
  // PRINT_FLAG_UTIL(fullbank);
  // PRINT_FLAG_UTIL(ignore_look);
  // PRINT_FLAG_UTIL(wrap);
  // PRINT_FLAG_UTIL(unwrap);
  // PRINT_FLAG_UTIL(topeffect);

  // Print NPC sale data for RL Tibia
  // for (const auto saleData : flags.npcsaledata())
  // {
  //   s << "\t" << saleData.name() << " (" << saleData.location() << "): sells_for=" << saleData.sale_price() << ", buys_for=" << saleData.buy_price() << ", currency=" << saleData.currency() << std::endl;
  // }

  // PRINT_FLAG_UTIL(corpse);
  // PRINT_FLAG_UTIL(player_corpse);

  // #undef PRINT_FLAG_UTIL

  os << s.str();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const proto::SpriteAnimation &animation)
{
  std::ostringstream s;

  s << "default_start_phase: " << animation.default_start_phase() << ", ";
  s << "synchronized: " << animation.synchronized() << ", ";
  s << "random_start_phase: " << animation.random_start_phase() << ", ";

  s << "loop_type: ";

  switch (animation.loop_type())
  {
  case proto::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_PINGPONG:
    s << "ANIMATION_LOOP_TYPE_PINGPONG";
    break;
  case proto::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_INFINITE:
    s << "ANIMATION_LOOP_TYPE_INFINITE";
    break;
  case proto::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_COUNTED:
  default:
    s << "ANIMATION_LOOP_TYPE_COUNTED";
    break;
  }

  s << std::endl;

  s << "loop_count: " << animation.loop_count() << ", " << std::endl;
  s << "phases:" << std::endl;
  for (int i = 0; i < animation.sprite_phase_size(); ++i)
  {
    auto phase = animation.sprite_phase().at(i);
    s << "\t[" << phase.duration_min() << ", " << phase.duration_max() << "]" << std::endl;
  }

  os << s.str();
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const SpriteAnimation &animation)
{
  std::ostringstream s;

  s << "default_start_phase: " << animation.defaultStartPhase << ", ";
  s << "synchronized: " << animation.synchronized << ", ";
  s << "random_start_phase: " << animation.randomStartPhase << ", ";

  s << "loop_type: ";

  switch (animation.loopType)
  {
  case AnimationLoopType::PingPong:
    s << "ANIMATION_LOOP_TYPE_PINGPONG";
    break;
  case AnimationLoopType::Infinite:
    s << "ANIMATION_LOOP_TYPE_INFINITE";
    break;
  case AnimationLoopType::Counted:
  default:
    s << "ANIMATION_LOOP_TYPE_COUNTED";
    break;
  }

  s << std::endl;

  s << "loop_count: " << animation.loopCount << ", " << std::endl;
  s << "phases (" << animation.phases.size() << "):" << std::endl;
  for (int i = 0; i < animation.phases.size(); ++i)
  {
    auto phase = animation.phases.at(i);
    s << "\t[" << phase.minDuration << ", " << phase.maxDuration << "]" << std::endl;
  }

  os << s.str();
  return os;
}
