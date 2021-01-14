#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <unordered_map>

#include <sstream>
#include <variant>

#ifndef MESSAGES_WRAPPER_H
#define MESSAGES_WRAPPER_H

#pragma warning(push, 0)
#include "protobuf/appearances.pb.h"
#pragma warning(pop)

#endif // MESSAGES_WRAPPER_H

#include "../const.h"
#include "../creature.h"
#include "../debug.h"
#include "../position.h"
#include "../sprite_info.h"
#include "../util.h"
#include "appearance_types.h"
#include "texture_atlas.h"

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

enum FixedFrameGroup
{
    OutfitIdle = 0,
    OutfitMoving = 1,
    ObjectInitial = 2
};

struct FrameGroup
{
    FrameGroup(FixedFrameGroup fixedGroup, uint32_t id, SpriteInfo &&spriteInfo)
        : fixedGroup(fixedGroup), id(id), spriteInfo(std::move(spriteInfo)) {}

    FixedFrameGroup fixedGroup;
    uint32_t id;
    SpriteInfo spriteInfo;

    FrameGroup(const FrameGroup &) = delete;
    FrameGroup(FrameGroup &&) = default;
};

/*
  Convenience wrapper for a tibia::protobuf Appearance
*/
class ObjectAppearance
{
  public:
    ObjectAppearance(const proto::Appearance &protobufAppearance);

    ObjectAppearance(ObjectAppearance &&other) noexcept;
    ObjectAppearance &operator=(ObjectAppearance &&) = default;

    size_t spriteCount(uint32_t frameGroup) const;

    const uint32_t getSpriteId(uint32_t frameGroup, int index) const;
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

    QuadrantRenderType quadrantRenderType = QuadrantRenderType::Full;

    // Appearance flag data
    struct AppearanceFlagData
    {
        // This is stored as bank.waypoints in the appearance protobuf file.
        uint32_t groundSpeed;
        uint32_t maxTextLength;
        uint32_t maxTextLengthOnce;
        // Represents the radius of the emitted light.
        uint32_t brightness;
        uint32_t color;
        int elevation = 0;
        uint32_t shiftX = 0;
        uint32_t shiftY = 0;
        ItemSlot itemSlot = ItemSlot::None;
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
    std::vector<FrameGroup> frameGroups;
    AppearanceFlag flags;
};

class Appearances
{
    using AppearanceId = uint32_t;

  public:
    static void loadTextureAtlases(const std::filesystem::path catalogContentsPath, const std::filesystem::path assetFolder);

    static void loadAppearanceData(const std::filesystem::path path);
    static std::pair<bool, std::optional<std::string>> dumpSpriteFiles(const std::filesystem::path &assetFolder, const std::filesystem::path &destinationFolder);

    static SpriteAnimation parseSpriteAnimation(const proto::SpriteAnimation &animation);
    static SpriteInfo parseSpriteInfo(const proto::SpriteInfo &info);
    static SpriteInfo parseCreatureType(const proto::SpriteInfo &info);
    static CreatureType parseCreatureType(const proto::Appearance &appearance);

    inline static const vme_unordered_map<AppearanceId, ObjectAppearance> &objects();

    static bool hasObject(AppearanceId id)
    {
        return _objects.find(id) != _objects.end();
    }

    static ObjectAppearance &getObjectById(AppearanceId id)
    {
        return _objects.at(id);
    }

    static TextureAtlas *getTextureAtlas(const uint32_t spriteId);

    static bool isLoaded;

    static size_t textureAtlasCount();
    static size_t objectCount();

  private:
    static vme_unordered_map<AppearanceId, ObjectAppearance> _objects;

    /* 
		Used for quick retrieval of the correct spritesheet given a sprite ID.
		It stores the upper bound of the sprite ids in the sprite sheet.
	*/
    static std::set<uint32_t> catalogIndex;

    static vme_unordered_map<uint32_t, std::unique_ptr<TextureAtlas>> textureAtlases;

    /* 
		Used for quick retrieval of a texture atlas given a sprite ID.
		It stores the start and end sprite id in the sprite sheet.
	*/
    static std::vector<SpriteRange> textureAtlasSpriteRanges;
};

inline const vme_unordered_map<uint32_t, ObjectAppearance> &Appearances::objects()
{
    return _objects;
}

//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>Printing>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>

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

inline std::ostringstream stringify(const ObjectAppearance::AppearanceFlagData &flags)
{
    std::ostringstream s;

    s << "{\n";

#define WRITE_INT(flag)                                            \
    do                                                             \
    {                                                              \
        if (flags.flag != 0)                                       \
        {                                                          \
            s << "\t" << #flag << ": " << flags.flag << std::endl; \
        }                                                          \
    } while (false)

    WRITE_INT(groundSpeed);
    WRITE_INT(maxTextLength);
    WRITE_INT(maxTextLengthOnce);
    WRITE_INT(brightness);
    WRITE_INT(color);
    WRITE_INT(shiftX);
    WRITE_INT(shiftY);
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
    s << "\titemSlot: " << to_underlying(flags.itemSlot);

    s << "}" << std::endl;

    return s;
}

inline std::ostream &operator<<(std::ostream &os, const ObjectAppearance::AppearanceFlagData &flags)
{
    os << stringify(flags).str();
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const ObjectAppearance &appearance)
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

#define PRINT_FLAG_UTIL(a)                                      \
    do                                                          \
    {                                                           \
        if (flags.has_##a() && flags.##a())                     \
        {                                                       \
            s << "\t" << #a << ":" << flags.##a() << std::endl; \
        }                                                       \
    } while (false)

    if (flags.has_bank())
    {
        std::cout << "bank.waypoints: " << flags.bank().waypoints() << std::endl;
    }
    PRINT_FLAG_UTIL(clip);
    PRINT_FLAG_UTIL(bottom);
    PRINT_FLAG_UTIL(top);
    PRINT_FLAG_UTIL(container);
    PRINT_FLAG_UTIL(cumulative);
    PRINT_FLAG_UTIL(usable);
    PRINT_FLAG_UTIL(forceuse);
    PRINT_FLAG_UTIL(multiuse);
    PRINT_FLAG_UTIL(liquidpool);
    PRINT_FLAG_UTIL(unpass);
    PRINT_FLAG_UTIL(unmove);
    PRINT_FLAG_UTIL(unsight);
    PRINT_FLAG_UTIL(avoid);
    PRINT_FLAG_UTIL(no_movement_animation);
    PRINT_FLAG_UTIL(take);
    PRINT_FLAG_UTIL(liquidcontainer);
    PRINT_FLAG_UTIL(hang);
    if (flags.has_hook())
    {
        if (flags.hook().has_direction())
            s << "\thook: " << flags.hook().direction() << std::endl;
        else
            s << "\thook (no direction)" << std::endl;
    }

    PRINT_FLAG_UTIL(rotate);
    if (flags.has_light())
    {
        s << "\tlight: ("
          << "brightness: " << flags.light().brightness() << ", color: " << flags.light().color() << ")" << std::endl;
    }
    PRINT_FLAG_UTIL(dont_hide);
    PRINT_FLAG_UTIL(translucent);
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
    PRINT_FLAG_UTIL(lying_object);
    PRINT_FLAG_UTIL(animate_always);
    PRINT_FLAG_UTIL(fullbank);
    PRINT_FLAG_UTIL(ignore_look);
    PRINT_FLAG_UTIL(wrap);
    PRINT_FLAG_UTIL(unwrap);
    PRINT_FLAG_UTIL(topeffect);

    // Print NPC sale data for RL Tibia
    // for (const auto saleData : flags.npcsaledata())
    // {
    //   s << "\t" << saleData.name() << " (" << saleData.location() << "): sells_for=" << saleData.sale_price() << ", buys_for=" << saleData.buy_price() << ", currency=" << saleData.currency() << std::endl;
    // }

    PRINT_FLAG_UTIL(corpse);
    PRINT_FLAG_UTIL(player_corpse);

#undef PRINT_FLAG_UTIL

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
