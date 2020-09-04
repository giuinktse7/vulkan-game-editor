#include "appearances.h"

#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <memory>
#include <algorithm>
#include <set>

#include "../logger.h"

#include "../util.h"
#include "../file.h"

#include "texture_atlas.h"
#include "../time_point.h"

constexpr size_t ReservedTextureAtlasCount = 5000;

std::unordered_map<uint32_t, Appearance> Appearances::objects;
std::unordered_map<uint32_t, tibia::protobuf::appearances::Appearance> Appearances::outfits;

std::vector<SpriteRange> Appearances::textureAtlasSpriteRanges;
std::unordered_map<uint32_t, std::unique_ptr<TextureAtlas>> Appearances::textureAtlases;

bool Appearances::isLoaded;

void Appearances::loadAppearanceData(const std::filesystem::path path)
{
    TimePoint start;

    tibia::protobuf::appearances::Appearances parsed;

    {
        std::fstream input(path.c_str(), std::ios::in | std::ios::binary);
        if (!parsed.ParseFromIstream(&input))
        {
            auto absolutePath = std::filesystem::absolute(std::filesystem::path(path));

            std::stringstream s;
            s << "Failed to parse appearances file at " << absolutePath << "." << std::endl;
            ABORT_PROGRAM(s.str());
        }

        google::protobuf::ShutdownProtobufLibrary();
    }

    for (int i = 0; i < parsed.object_size(); ++i)
    {
        const tibia::protobuf::appearances::Appearance &object = parsed.object(i);
        auto info = object.frame_group().at(0).sprite_info();

        // if (object.id() == 3031 || object.id() == 103)
        // {
        //     std::cout << "\n(cid: " << object.id() << "): " << std::endl;
        //     std::cout << "info: " << info << std::endl;
        //     if (object.has_flags())
        //     {
        //         std::cout << "flags: " << object.flags() << std::endl;
        //     }
        // }

        // if (object.has_flags() && object.flags().has_cumulative() && object.flags().cumulative() && info.sprite_id_size() > 1 && info.has_animation())
        // {
        //     std::cout << "\n(cid: " << object.id() << "): " << std::endl;
        //     std::cout << "framegroups: " << object.frame_group_size() << std::endl;
        //     std::cout << "info: " << info << std::endl;
        //     if (object.has_flags())
        //     {
        //         std::cout << "flags: " << std::endl;
        //         std::cout << object.flags() << std::endl;
        //     }
        //     if (info.has_animation())
        //     {
        //         std::cout << "Animation:" << std::endl
        //                   << info.animation() << std::endl;
        //     }
        //     ++total;
        // }
        Appearances::objects.emplace(object.id(), object);
    }

    // std::cout << "Total: " << total << std::endl;

    for (int i = 0; i < parsed.outfit_size(); ++i)
    {
        const tibia::protobuf::appearances::Appearance &outfit = parsed.outfit(i);
        Appearances::outfits[outfit.id()] = outfit;
    }

    Appearances::isLoaded = true;

    VME_LOG("Loaded appearances.dat in " << start.elapsedMillis() << " ms.");
}

TextureAtlas *Appearances::getTextureAtlas(const uint32_t spriteId)
{
    size_t i = textureAtlasSpriteRanges.size() >> 1;

    // Perform a binary search to find the Texture atlas containing the spriteId.

    SpriteRange range = textureAtlasSpriteRanges[i];
    size_t change = (textureAtlasSpriteRanges.size() >> 2) + (textureAtlasSpriteRanges.size() & 1);

    while (change != 0)
    {
        if (range.start > spriteId)
        {
            i -= change;
        }
        else if (range.end < spriteId)
        {
            i += change;
        }
        else
        {
            return textureAtlases.at(range.end).get();
        }

        range = textureAtlasSpriteRanges[i];
        change = (change >> 1) + (change & 1);
    }

    ABORT_PROGRAM("There is no sprite with ID " + std::to_string(spriteId));
}

void Appearances::loadTextureAtlases(const std::filesystem::path catalogContentsPath)
{
    TimePoint start;

    if (!std::filesystem::exists(catalogContentsPath))
    {
        std::stringstream s;
        s << "Could not locate the catalog JSON file. Failed to find file at path: " + std::filesystem::absolute(catalogContentsPath).u8string() << std::endl;
        ABORT_PROGRAM(s.str());
    }

    std::ifstream fileStream(catalogContentsPath);
    nlohmann::json catalogJson;
    fileStream >> catalogJson;
    fileStream.close();

    std::filesystem::path basepath("C:/Users/giuin/AppData/Local/Tibia11/packages/Tibia/assets");

    textureAtlasSpriteRanges.reserve(5000);

    for (const auto &entry : catalogJson)
    {
        if (entry.at("type") == "sprite")
        {
            std::string filename = entry.at("file");
            SpriteLayout spriteType = entry.at("spritetype");
            uint32_t firstSpriteId = entry.at("firstspriteid");
            uint32_t lastSpriteId = entry.at("lastspriteid");
            // uint8_t area = entry.at("area");

            std::filesystem::path filePath(filename);
            std::filesystem::path absolutePath = basepath / filePath;

            std::vector<uint8_t> buffer = File::read(absolutePath.string());

            Appearances::textureAtlases[lastSpriteId] = std::make_unique<TextureAtlas>(
                lastSpriteId,
                buffer,
                TextureAtlasSize.width,
                TextureAtlasSize.height,
                firstSpriteId,
                spriteType,
                filename);

            Appearances::textureAtlasSpriteRanges.emplace_back<SpriteRange>({firstSpriteId, lastSpriteId});
        }
    }

    textureAtlasSpriteRanges.shrink_to_fit();

    /*
        This sort is necessary! It enables lower_bound in getTextureAtlas()
        to have complexity O(lg n).
    */
    std::sort(
        Appearances::textureAtlasSpriteRanges.begin(),
        Appearances::textureAtlasSpriteRanges.end(),
        [](SpriteRange a, SpriteRange b) { return a.start < b.start; });

    VME_LOG("Loaded compressed sprites in " << start.elapsedMillis() << " ms.");
}

SpriteInfo SpriteInfo::fromProtobufData(tibia::protobuf::appearances::SpriteInfo spriteInfo)
{
    SpriteInfo info{};
    if (spriteInfo.has_animation())
    {
        info.animation = std::make_unique<SpriteAnimation>(SpriteAnimation::fromProtobufData(spriteInfo.animation()));
    }
    info.boundingSquare = spriteInfo.bounding_square();
    info.isOpaque = spriteInfo.bounding_square();
    info.layers = spriteInfo.layers();
    info.patternWidth = spriteInfo.pattern_width();
    info.patternHeight = spriteInfo.pattern_height();
    info.patternDepth = spriteInfo.pattern_depth();

    for (auto id : spriteInfo.sprite_id())
    {
        info.spriteIds.emplace_back<uint32_t &>(id);
    }

    return info;
}

SpriteAnimation *SpriteInfo::getAnimation() const
{
    return animation.get();
}

SpriteAnimation SpriteAnimation::fromProtobufData(tibia::protobuf::appearances::SpriteAnimation animation)
{
    SpriteAnimation anim{};
    anim.defaultStartPhase = animation.default_start_phase();
    anim.loopCount = animation.loop_count();

    switch (animation.loop_type())
    {
    case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_PINGPONG:
        anim.loopType = AnimationLoopType::PingPong;
        break;
    case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_COUNTED:
        anim.loopType = AnimationLoopType::Counted;
        break;
    case tibia::protobuf::shared::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_INFINITE:
    default:
        anim.loopType = AnimationLoopType::Infinite;
        break;
    }

    for (auto phase : animation.sprite_phase())
    {
        anim.phases.push_back({phase.duration_min(), phase.duration_max()});
    }

    if (animation.has_default_start_phase())
    {
        anim.defaultStartPhase = animation.default_start_phase();
    }
    if (animation.has_random_start_phase())
    {
        anim.randomStartPhase = animation.random_start_phase();
    }
    if (animation.has_synchronized())
    {
        anim.synchronized = animation.synchronized();
    }

    return anim;
}

/*
    Constructs an Appearance from protobuf Appearance data.
*/
Appearance::Appearance(tibia::protobuf::appearances::Appearance protobufAppearance)
{
    this->clientId = protobufAppearance.id();
    this->name = protobufAppearance.name();

    // if (this->clientId == 447 || this->clientId == 5750)
    // {
    //     std::cout << protobufAppearance.flags() << std::endl;
    // }

    if (protobufAppearance.frame_group_size() == 1)
    {
        this->appearanceData = SpriteInfo::fromProtobufData(protobufAppearance.frame_group().at(0).sprite_info());
    }

    // Flags
    this->flags = static_cast<AppearanceFlag>(0);
    if (protobufAppearance.has_flags())
    {
        auto flags = protobufAppearance.flags();

#define ADD_FLAG_UTIL(flagType, flag) \
    do                                \
    {                                 \
        if (flags.has_##flagType())   \
        {                             \
            this->flags |= flag;      \
        }                             \
    } while (false)

        ADD_FLAG_UTIL(bank, AppearanceFlag::Bank);
        ADD_FLAG_UTIL(clip, AppearanceFlag::GroundBorder);
        ADD_FLAG_UTIL(bottom, AppearanceFlag::Bottom);
        ADD_FLAG_UTIL(top, AppearanceFlag::Top);
        ADD_FLAG_UTIL(container, AppearanceFlag::Container);
        ADD_FLAG_UTIL(cumulative, AppearanceFlag::Cumulative);
        ADD_FLAG_UTIL(usable, AppearanceFlag::Usable);
        ADD_FLAG_UTIL(forceuse, AppearanceFlag::Forceuse);
        ADD_FLAG_UTIL(multiuse, AppearanceFlag::Multiuse);
        ADD_FLAG_UTIL(write, AppearanceFlag::Write);
        ADD_FLAG_UTIL(write_once, AppearanceFlag::WriteOnce);
        ADD_FLAG_UTIL(liquidpool, AppearanceFlag::Liquidpool);
        ADD_FLAG_UTIL(unpass, AppearanceFlag::Unpass);
        ADD_FLAG_UTIL(unmove, AppearanceFlag::Unmove);
        ADD_FLAG_UTIL(unsight, AppearanceFlag::Unsight);
        ADD_FLAG_UTIL(avoid, AppearanceFlag::Avoid);
        ADD_FLAG_UTIL(no_movement_animation, AppearanceFlag::NoMovementAnimation);
        ADD_FLAG_UTIL(take, AppearanceFlag::Take);
        ADD_FLAG_UTIL(liquidcontainer, AppearanceFlag::LiquidContainer);
        ADD_FLAG_UTIL(hang, AppearanceFlag::Hang);
        ADD_FLAG_UTIL(hook, AppearanceFlag::Hook);
        ADD_FLAG_UTIL(rotate, AppearanceFlag::Rotate);
        ADD_FLAG_UTIL(light, AppearanceFlag::Light);
        ADD_FLAG_UTIL(dont_hide, AppearanceFlag::DontHide);
        ADD_FLAG_UTIL(translucent, AppearanceFlag::Translucent);
        ADD_FLAG_UTIL(shift, AppearanceFlag::Shift);
        ADD_FLAG_UTIL(height, AppearanceFlag::Height);
        ADD_FLAG_UTIL(lying_object, AppearanceFlag::LyingObject);
        ADD_FLAG_UTIL(animate_always, AppearanceFlag::AnimateAlways);
        ADD_FLAG_UTIL(automap, AppearanceFlag::Automap);
        ADD_FLAG_UTIL(lenshelp, AppearanceFlag::Lenshelp);
        ADD_FLAG_UTIL(fullbank, AppearanceFlag::Fullbank);
        ADD_FLAG_UTIL(ignore_look, AppearanceFlag::IgnoreLook);
        ADD_FLAG_UTIL(clothes, AppearanceFlag::Clothes);
        ADD_FLAG_UTIL(default_action, AppearanceFlag::DefaultAction);
        ADD_FLAG_UTIL(market, AppearanceFlag::Market);
        ADD_FLAG_UTIL(wrap, AppearanceFlag::Wrap);
        ADD_FLAG_UTIL(unwrap, AppearanceFlag::Unwrap);
        ADD_FLAG_UTIL(topeffect, AppearanceFlag::Topeffect);
        // TODO npcsaledata flag is not handled right now (probably not needed)
        ADD_FLAG_UTIL(changedtoexpire, AppearanceFlag::ChangedToExpire);
        ADD_FLAG_UTIL(corpse, AppearanceFlag::Corpse);
        ADD_FLAG_UTIL(player_corpse, AppearanceFlag::PlayerCorpse);
        ADD_FLAG_UTIL(cyclopediaitem, AppearanceFlag::CyclopediaItem);

#undef ADD_FLAG_UTIL

        if (hasFlag(AppearanceFlag::Bank))
            flagData.bankWaypoints = flags.bank().waypoints();
        if (hasFlag(AppearanceFlag::Write))
            flagData.maxTextLength = flags.write().max_text_length();
        if (hasFlag(AppearanceFlag::WriteOnce))
            flagData.maxTextLengthOnce = flags.write_once().max_text_length_once();
        if (hasFlag(AppearanceFlag::Hook))
        {
            auto direction = flags.hook().direction();
            if (direction == tibia::protobuf::shared::HOOK_TYPE::HOOK_TYPE_SOUTH)
                flagData.hookDirection = HookType::South;
            else
                flagData.hookDirection = HookType::East;
        }
        if (hasFlag(AppearanceFlag::Light))
        {
            flagData.brightness = flags.light().brightness();
            flagData.color = flags.light().color();
        }

        if (hasFlag(AppearanceFlag::Shift))
        {
            if (flags.shift().has_x())
                flagData.shiftX = flags.shift().x();
            if (flags.shift().has_y())
                flagData.shiftY = flags.shift().y();
        }
        if (hasFlag(AppearanceFlag::Height))
            flagData.elevation = flags.height().elevation();
        if (hasFlag(AppearanceFlag::Automap))
            flagData.automapColor = flags.automap().color();
        if (hasFlag(AppearanceFlag::Lenshelp))
            flagData.lenshelp = flags.lenshelp().id();
        if (hasFlag(AppearanceFlag::Clothes))
            flagData.itemSlot = flags.clothes().slot();
        if (hasFlag(AppearanceFlag::DefaultAction))
        {
            switch (flags.default_action().action())
            {

            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_LOOK:
                flagData.defaultAction = AppearancePlayerDefaultAction::Look;
                break;
            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_USE:
                flagData.defaultAction = AppearancePlayerDefaultAction::Use;
                break;
            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_OPEN:
                flagData.defaultAction = AppearancePlayerDefaultAction::Open;
                break;
            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_AUTOWALK_HIGHLIGHT:
                flagData.defaultAction = AppearancePlayerDefaultAction::AutowalkHighlight;
                break;
            case tibia::protobuf::shared::PLAYER_ACTION::PLAYER_ACTION_NONE:
            default:
                flagData.defaultAction = AppearancePlayerDefaultAction::None;
                break;
            }
        }
        if (hasFlag(AppearanceFlag::Market))
        {
#define MAP_MARKET_FLAG(src, dst)                                     \
    if (1)                                                            \
    {                                                                 \
    case tibia::protobuf::shared::ITEM_CATEGORY::ITEM_CATEGORY_##src: \
        flagData.market.category = dst;                               \
        break;                                                        \
    }                                                                 \
    else

            switch (flags.market().category())
            {
                MAP_MARKET_FLAG(ARMORS, AppearanceItemCategory::Armors);
                MAP_MARKET_FLAG(AMULETS, AppearanceItemCategory::Amulets);
                MAP_MARKET_FLAG(BOOTS, AppearanceItemCategory::Boots);
                MAP_MARKET_FLAG(CONTAINERS, AppearanceItemCategory::Containers);
                MAP_MARKET_FLAG(DECORATION, AppearanceItemCategory::Decoration);
                MAP_MARKET_FLAG(FOOD, AppearanceItemCategory::Food);
                MAP_MARKET_FLAG(HELMETS_HATS, AppearanceItemCategory::HelmetsHats);
                MAP_MARKET_FLAG(LEGS, AppearanceItemCategory::Legs);
                MAP_MARKET_FLAG(OTHERS, AppearanceItemCategory::Others);
                MAP_MARKET_FLAG(POTIONS, AppearanceItemCategory::Potions);
                MAP_MARKET_FLAG(RINGS, AppearanceItemCategory::Rings);
                MAP_MARKET_FLAG(RUNES, AppearanceItemCategory::Runes);
                MAP_MARKET_FLAG(SHIELDS, AppearanceItemCategory::Shields);
                MAP_MARKET_FLAG(TOOLS, AppearanceItemCategory::Tools);
                MAP_MARKET_FLAG(VALUABLES, AppearanceItemCategory::Valuables);
                MAP_MARKET_FLAG(AMMUNITION, AppearanceItemCategory::Ammunition);
                MAP_MARKET_FLAG(AXES, AppearanceItemCategory::Axes);
                MAP_MARKET_FLAG(CLUBS, AppearanceItemCategory::Clubs);
                MAP_MARKET_FLAG(DISTANCE_WEAPONS, AppearanceItemCategory::DistanceWeapons);
                MAP_MARKET_FLAG(SWORDS, AppearanceItemCategory::Swords);
                MAP_MARKET_FLAG(WANDS_RODS, AppearanceItemCategory::WandsRods);
                MAP_MARKET_FLAG(PREMIUM_SCROLLS, AppearanceItemCategory::PremiumScrolls);
                MAP_MARKET_FLAG(TIBIA_COINS, AppearanceItemCategory::TibiaCoins);
                MAP_MARKET_FLAG(CREATURE_PRODUCTS, AppearanceItemCategory::CreatureProducts);
            default:
                ABORT_PROGRAM("Unknown appearance flag market category: " + std::to_string(flags.market().category()));
                break;
            }

#undef MAP_MARKET_FLAG
        }
        // This flag is probably not necessary in a map editor
        // if (hasFlag(AppearanceFlag::NpcSaleData))
        // {
        // }
        if (hasFlag(AppearanceFlag::ChangedToExpire))
            flagData.changedToExpireFormerObjectTypeId = flags.changedtoexpire().former_object_typeid();
        if (hasFlag(AppearanceFlag::CyclopediaItem))
            flagData.cyclopediaClientId = flags.cyclopediaitem().cyclopedia_type();
    }
}

uint32_t Appearance::getFirstSpriteId() const
{
    return getSpriteInfo().spriteIds.at(0);
}

const SpriteInfo &Appearance::getSpriteInfo(size_t frameGroup) const
{
    if (std::holds_alternative<SpriteInfo>(appearanceData))
    {
        return std::get<SpriteInfo>(appearanceData);
    }
    else
    {
        return std::get<std::vector<FrameGroup>>(appearanceData).at(frameGroup).spriteInfo;
    }
}

const SpriteInfo &Appearance::getSpriteInfo() const
{
    return getSpriteInfo(0);
}

size_t Appearance::frameGroupCount() const
{
    if (std::holds_alternative<SpriteInfo>(appearanceData))
    {
        return 1;
    }

    return std::get<std::vector<FrameGroup>>(appearanceData).size();
}
