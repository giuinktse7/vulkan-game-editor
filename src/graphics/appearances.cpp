#include "appearances.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_map>

#include "../logger.h"

#include "../file.h"
#include "../util.h"

#include "../time_point.h"
#include "texture_atlas.h"

vme_unordered_map<uint32_t, Appearance> Appearances::objects;
vme_unordered_map<uint32_t, proto::Appearance> Appearances::outfits;

std::vector<SpriteRange> Appearances::textureAtlasSpriteRanges;
vme_unordered_map<uint32_t, std::unique_ptr<TextureAtlas>> Appearances::textureAtlases;

bool Appearances::isLoaded;

size_t Appearances::textureAtlasCount()
{
    return textureAtlases.size();
}

void Appearances::loadAppearanceData(const std::filesystem::path path)
{
    TimePoint start;

    proto::Appearances parsed;

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
        const proto::Appearance &object = parsed.object(i);
        // auto info = object.frame_group().at(0).sprite_info();

        // if (object.id() == 675)
        // {
        //     auto info = object.frame_group().at(0).sprite_info();
        //     VME_LOG_D(info.layers());
        //     VME_LOG_D(info.sprite_id_size());
        // }

        Appearances::objects.emplace(object.id(), std::move(object));
    }

    // std::cout << "Total: " << total << std::endl;

    for (int i = 0; i < parsed.outfit_size(); ++i)
    {
        const proto::Appearance &outfit = parsed.outfit(i);
        Appearances::outfits[outfit.id()] = std::move(outfit);
    }

    Appearances::isLoaded = true;

    VME_LOG("Loaded appearances.dat in " << start.elapsedMillis() << " ms.");
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

    /* 
      NOTE: This id (Texture atlas ID) is used to index texture resources (see MapRenderer::textureResources).
      Because it is used to index a vector in the renderer, it has to stay within [0, amountOfTextureAtlases)
  */
    uint32_t id = 0;
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

            LZMACompressedBuffer compressedBuffer;
            compressedBuffer.buffer = File::read(absolutePath.string());

            Appearances::textureAtlases[lastSpriteId] = std::make_unique<TextureAtlas>(
                id,
                std::move(compressedBuffer),
                TextureAtlasSize.width,
                TextureAtlasSize.height,
                firstSpriteId,
                lastSpriteId,
                spriteType,
                filename);

            Appearances::textureAtlasSpriteRanges.emplace_back<SpriteRange>({firstSpriteId, lastSpriteId});

            ++id;
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

    VME_LOG("Loaded compressed texture atlases in " << start.elapsedMillis() << " ms.");
}

std::pair<bool, std::optional<std::string>> Appearances::dumpSpriteFiles(const std::filesystem::path &assetFolder, const std::filesystem::path &destinationFolder)
{
    namespace fs = std::filesystem;
    bool validAssetDirectory = fs::exists(assetFolder) && fs::is_directory(assetFolder);
    if (!validAssetDirectory)
    {
        std::ostringstream s;
        s << "Invalid asset directory: " << assetFolder;
        return {false, s.str()};
    }

    if (fs::exists(destinationFolder) && !fs::is_directory(destinationFolder))
    {
        std::ostringstream s;
        s << "Destination folder must be a directory. Bad destination: " << destinationFolder;
        return {false, s.str()};
    }

    if (!fs::exists(destinationFolder))
    {
        bool created = fs::create_directory(destinationFolder);
        if (!created)
        {
            return {false, "Could not create destination folder."};
        }
    }

    for (const auto &entry : fs::directory_iterator(assetFolder))
    {
        if (entry.path().extension() == ".lzma")
        {
            auto filepath = (destinationFolder / entry.path().stem());
            auto file = File::read(entry.path());
            auto decompressed = LZMA::decompress(std::move(file));

            File::write(filepath, std::move(decompressed));
            VME_LOG("Wrote " << filepath);
        }
    }

    return {true, std::nullopt};
}

TextureAtlas *Appearances::getTextureAtlas(const uint32_t spriteId)
{
    // TODO Checks #1 and #2 should not be necessary. This algorithm can
    // TODO  probably be improved, but it works for now.

    // TODO #1
    if (textureAtlasSpriteRanges.back().start <= spriteId)
    {
        auto end = textureAtlasSpriteRanges.back().end;
        DEBUG_ASSERT(spriteId <= end, "spriteId is not present in any texture atlas.");

        return textureAtlases.at(end).get();
    }

    size_t last = textureAtlasSpriteRanges.size() - 1;
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
        // TODO #2 (The call to std::min())
        range = textureAtlasSpriteRanges[std::min(i, last)];
        change = (change >> 1) + (change & 1);
    }

    ABORT_PROGRAM("There is no sprite with ID " + std::to_string(spriteId));
}

SpriteInfo SpriteInfo::fromProtobufData(proto::SpriteInfo spriteInfo, bool cumulative)
{
    SpriteInfo info{};
    if (spriteInfo.has_animation())
    {
        info._animation = std::make_unique<SpriteAnimation>(SpriteAnimation::fromProtobufData(spriteInfo.animation()));
    }
    info.boundingSquare = spriteInfo.bounding_square();
    info.isOpaque = spriteInfo.bounding_square();
    info.layers = spriteInfo.layers();
    info.patternWidth = spriteInfo.pattern_width();
    info.patternHeight = spriteInfo.pattern_height();
    info.patternDepth = spriteInfo.pattern_depth();
    info.patternSize = info.patternWidth * info.patternHeight * info.patternDepth;

    for (auto id : spriteInfo.sprite_id())
    {
        info.spriteIds.emplace_back(id);
    }

    return info;
}

SpriteAnimation *SpriteInfo::animation() const
{
    return _animation.get();
}

SpriteAnimation SpriteAnimation::fromProtobufData(proto::SpriteAnimation animation)
{
    SpriteAnimation anim{};
    anim.defaultStartPhase = animation.default_start_phase();
    anim.loopCount = animation.loop_count();

    switch (animation.loop_type())
    {
    case proto::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_PINGPONG:
        anim.loopType = AnimationLoopType::PingPong;
        break;
    case proto::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_COUNTED:
        anim.loopType = AnimationLoopType::Counted;
        break;
    case proto::ANIMATION_LOOP_TYPE::ANIMATION_LOOP_TYPE_INFINITE:
    default:
        anim.loopType = AnimationLoopType::Infinite;
        break;
    }

    for (auto phase : animation.sprite_phase())
    {
        anim.phases.emplace_back<SpritePhase>({phase.duration_min(), phase.duration_max()});
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
Appearance::Appearance(proto::Appearance protobufAppearance)
{
    this->clientId = protobufAppearance.id();
    this->name = protobufAppearance.name();

    // if (this->clientId == 447 || this->clientId == 5750)
    // {
    //     std::cout << protobufAppearance.flags() << std::endl;
    // }

    if (protobufAppearance.frame_group_size() == 1)
    {
        const auto &spriteInfo = protobufAppearance.frame_group().at(0).sprite_info();
        bool cumulative = protobufAppearance.flags().has_cumulative() && protobufAppearance.flags().cumulative();

        auto group = protobufAppearance.frame_group(0);
        frameGroups.emplace_back<FrameGroup>({static_cast<FixedFrameGroup>(group.fixed_frame_group()),
                                              static_cast<uint32_t>(group.id()),
                                              SpriteInfo::fromProtobufData(spriteInfo, cumulative)});
    }
    else
    {
        VME_LOG_D("More than one frame group for clientId: " << protobufAppearance.id());
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
            if (direction == proto::HOOK_TYPE::HOOK_TYPE_SOUTH)
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

            case proto::PLAYER_ACTION::PLAYER_ACTION_LOOK:
                flagData.defaultAction = AppearancePlayerDefaultAction::Look;
                break;
            case proto::PLAYER_ACTION::PLAYER_ACTION_USE:
                flagData.defaultAction = AppearancePlayerDefaultAction::Use;
                break;
            case proto::PLAYER_ACTION::PLAYER_ACTION_OPEN:
                flagData.defaultAction = AppearancePlayerDefaultAction::Open;
                break;
            case proto::PLAYER_ACTION::PLAYER_ACTION_AUTOWALK_HIGHLIGHT:
                flagData.defaultAction = AppearancePlayerDefaultAction::AutowalkHighlight;
                break;
            case proto::PLAYER_ACTION::PLAYER_ACTION_NONE:
            default:
                flagData.defaultAction = AppearancePlayerDefaultAction::None;
                break;
            }
        }
        if (hasFlag(AppearanceFlag::Market))
        {
#define MAP_MARKET_FLAG(src, dst)                   \
    if (1)                                          \
    {                                               \
    case proto::ITEM_CATEGORY::ITEM_CATEGORY_##src: \
        flagData.market.category = dst;             \
        break;                                      \
    }                                               \
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

Appearance::Appearance(Appearance &&other) noexcept
    : clientId(other.clientId),
      name(std::move(other.name)),
      flagData(std::move(other.flagData)),
      frameGroups(std::move(other.frameGroups)),
      flags(std::move(other.flags)) {}

uint32_t Appearance::getFirstSpriteId() const
{
    return getSpriteInfo().spriteIds.at(0);
}

const SpriteInfo &Appearance::getSpriteInfo(size_t frameGroup) const
{
    return frameGroups.at(frameGroup).spriteInfo;
}

const SpriteInfo &Appearance::getSpriteInfo() const
{
    return getSpriteInfo(0);
}

const uint32_t Appearance::getSpriteId(uint32_t frameGroup, int index) const
{
    return getSpriteInfo(frameGroup).spriteIds.at(index);
}

size_t Appearance::spriteCount(uint32_t frameGroup) const
{
    return this->getSpriteInfo(frameGroup).spriteIds.size();
}

size_t Appearance::frameGroupCount() const
{
    return frameGroups.size();
}
