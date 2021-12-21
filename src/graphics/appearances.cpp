#include "appearances.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <set>
#include <string>
#include <unordered_map>

#include "../file.h"
#include "../logger.h"
#include "../time_util.h"
#include "../util.h"
#include "texture_atlas.h"

vme_unordered_map<uint32_t, ObjectAppearance> Appearances::_objects;
vme_unordered_map<uint32_t, CreatureAppearance> Appearances::_creatures;

std::vector<SpriteRange> Appearances::textureAtlasSpriteRanges;
vme_unordered_map<uint32_t, std::unique_ptr<TextureAtlas>> Appearances::textureAtlases;

bool Appearances::isLoaded;

void Appearances::loadAppearanceData(const std::filesystem::path path)
{
    TimePoint start;

    proto::Appearances parsed;

    {
        std::fstream input(path, std::ios::in | std::ios::binary);
        bool success = parsed.ParseFromIstream(&input);
        google::protobuf::ShutdownProtobufLibrary();

        if (!success)
        {
            auto absolutePath = std::filesystem::absolute(std::filesystem::path(path));

            std::stringstream s;
            s << "Failed to parse appearances file at " << absolutePath << "." << std::endl;
            ABORT_PROGRAM(s.str());
        }
    }

    TimePoint startObjects;
    for (int i = 0; i < parsed.object_size(); ++i)
    {
        const proto::Appearance &object = parsed.object(i);
        Appearances::_objects.emplace(object.id(), object);
    }
    auto objectsMs = startObjects.elapsedMillis();

    TimePoint startOutfits;
    for (int i = 0; i < parsed.outfit_size(); ++i)
    {
        auto &creatureAppearance = parsed.outfit(i);
        Appearances::_creatures.emplace(creatureAppearance.id(), creatureAppearance);
    }
    auto outfitsMs = startOutfits.elapsedMillis();

    VME_LOG("Loaded appearances.dat in " << start.elapsedMillis() << " ms (objects: " << objectsMs << " ms, "
                                         << "outfits: " << outfitsMs << " ms).");

    Appearances::isLoaded = true;
}

void Appearances::loadTextureAtlases(const std::filesystem::path catalogContentsPath, const std::filesystem::path assetFolder)
{
    TimePoint start;

    if (!std::filesystem::exists(catalogContentsPath))
    {
        std::stringstream s;
        s << "Could not locate the catalog JSON file. Failed to find file at path: " + std::filesystem::absolute(catalogContentsPath).string() << std::endl;
        ABORT_PROGRAM(s.str());
    }

    std::ifstream fileStream(catalogContentsPath);
    nlohmann::json catalogJson;
    fileStream >> catalogJson;
    fileStream.close();

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
            std::filesystem::path absolutePath = assetFolder / filePath;

            LZMACompressedBuffer compressedBuffer;
            compressedBuffer.buffer = File::read(absolutePath.string());

            Appearances::textureAtlases[lastSpriteId] = std::make_unique<TextureAtlas>(
                std::move(compressedBuffer),
                TextureAtlasSize.width,
                TextureAtlasSize.height,
                firstSpriteId,
                lastSpriteId,
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
            // Handle negative i
            if (change > i)
            {
                i = 0;
            }
            else
            {
                i -= change;
            }
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

size_t Appearances::textureAtlasCount()
{
    return textureAtlases.size();
}

size_t Appearances::objectCount()
{
    return Appearances::_objects.size();
}

SpriteInfo Appearances::parseSpriteInfo(const proto::SpriteInfo &spriteInfo)
{
    SpriteInfo info = spriteInfo.has_animation() ? SpriteInfo(Appearances::parseSpriteAnimation(spriteInfo.animation())) : SpriteInfo();
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

SpriteAnimation Appearances::parseSpriteAnimation(const proto::SpriteAnimation &animation)
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
ObjectAppearance::ObjectAppearance(const proto::Appearance &protobufAppearance)
{
    this->clientId = protobufAppearance.id();
    this->setName(protobufAppearance.name());

    if (protobufAppearance.frame_group_size() == 1)
    {
        const auto &spriteInfo = protobufAppearance.frame_group().at(0).sprite_info();
        // bool cumulative = protobufAppearance.flags().has_cumulative() && protobufAppearance.flags().cumulative();

        auto group = protobufAppearance.frame_group(0);
        _frameGroups.emplace_back(static_cast<FixedFrameGroup>(group.fixed_frame_group()),
                                  static_cast<uint32_t>(group.id()),
                                  Appearances::parseSpriteInfo(spriteInfo));
    }
    else
    {
        VME_LOG_D("More than one frame group for object with clientId: " << protobufAppearance.id());
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

        ADD_FLAG_UTIL(bank, AppearanceFlag::Ground);
        ADD_FLAG_UTIL(clip, AppearanceFlag::Border);
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

        if (hasFlag(AppearanceFlag::Ground))
            flagData.groundSpeed = flags.bank().waypoints();
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
        {
            uint32_t slot = flags.clothes().slot();
            DEBUG_ASSERT(slot <= 12, "Invalid slot");
            flagData.itemSlot = static_cast<ItemSlot>(slot);
        }

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
#define MAP_MARKET_FLAG(src, dst)                       \
    if (1)                                              \
    {                                                   \
        case proto::ITEM_CATEGORY::ITEM_CATEGORY_##src: \
            flagData.market.category = dst;             \
            break;                                      \
    }                                                   \
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

ObjectAppearance::ObjectAppearance(ObjectAppearance &&other) noexcept
    : clientId(other.clientId),
      _name(std::move(other._name)),
      flagData(std::move(other.flagData)),
      _frameGroups(std::move(other._frameGroups)),
      flags(std::move(other.flags)) {}

void ObjectAppearance::cacheTextureAtlases()
{
    for (int frameGroup = 0; frameGroup < frameGroupCount(); ++frameGroup)
    {
        for (const auto spriteId : getSpriteInfo(frameGroup).spriteIds)
        {
            // Stop if the cache is full
            if (_atlases.back() != nullptr)
            {
                return;
            }
            cacheTextureAtlas(spriteId);
        }
    }
}

void ObjectAppearance::cacheTextureAtlas(uint32_t spriteId)
{
    // If nothing is cached, cache the TextureAtlas for the first sprite ID in the appearance.
    if (_atlases.front() == nullptr)
        _atlases.front() = Appearances::getTextureAtlas(getFirstSpriteId());

    for (int i = 0; i < _atlases.size(); ++i)
    {
        TextureAtlas *&atlas = _atlases[i];
        // End of current cache reached, caching the atlas
        if (atlas == nullptr)
        {
            atlas = Appearances::getTextureAtlas(spriteId);
            return;
        }
        else
        {
            if (atlas->firstSpriteId <= spriteId && spriteId <= atlas->lastSpriteId)
            {
                // The TextureAtlas is already cached
                return;
            }
        }
    }
}

uint32_t ObjectAppearance::getFirstSpriteId() const
{
    return getSpriteInfo().spriteIds.at(0);
}

const SpriteInfo &ObjectAppearance::getSpriteInfo(size_t frameGroup) const
{
    return _frameGroups.at(frameGroup).spriteInfo;
}

const SpriteInfo &ObjectAppearance::getSpriteInfo() const
{
    return getSpriteInfo(0);
}

const uint32_t ObjectAppearance::getSpriteId(uint32_t frameGroup, int index) const
{
    return getSpriteInfo(frameGroup).spriteIds.at(index);
}

size_t ObjectAppearance::spriteCount(uint32_t frameGroup) const
{
    return this->getSpriteInfo(frameGroup).spriteIds.size();
}

size_t ObjectAppearance::frameGroupCount() const
{
    return _frameGroups.size();
}

const std::vector<FrameGroup> &ObjectAppearance::frameGroups() const noexcept
{
    return _frameGroups;
}

const std::string &ObjectAppearance::name() const noexcept
{
    return _name;
}

void ObjectAppearance::setName(std::string name)
{
    _name = name;
}

CreatureAppearance::CreatureAppearance(const proto::Appearance &appearance)
    : _id(appearance.id())
{
    _frameGroups.reserve(appearance.frame_group_size());
    for (int i = 0; i < appearance.frame_group_size(); ++i)
    {
        const auto frameGroup = appearance.frame_group().at(i);
        const auto &spriteInfo = frameGroup.sprite_info();

        _frameGroups.emplace_back<FrameGroup>({static_cast<FixedFrameGroup>(frameGroup.fixed_frame_group()),
                                               static_cast<uint32_t>(frameGroup.id()),
                                               Appearances::parseSpriteInfo(spriteInfo)});
    }
}

uint8_t getPostureIdByIndex(const FrameGroup &fg, uint32_t spriteIndex)
{
    uint8_t directions = fg.spriteInfo.patternWidth;
    uint8_t addons = fg.spriteInfo.patternHeight;
    uint8_t layers = fg.spriteInfo.layers;

    return spriteIndex / (layers * directions * addons);
}

const TextureInfo CreatureAppearance::getTextureInfo(uint32_t frameGroupId, uint32_t spriteIndex, TextureInfo::CoordinateType coordinateType) const
{
    auto &fg = this->frameGroup(frameGroupId);
    uint8_t layers = fg.spriteInfo.layers;

    uint32_t spriteId = fg.getSpriteId(spriteIndex);

    TextureAtlas *atlas = getTextureAtlas(spriteId);

    TextureInfo info;
    info.atlas = atlas;
    info.window = atlas->getTextureWindow(spriteId, coordinateType);

    if (coordinateType == TextureInfo::CoordinateType::Unnormalized)
    {
        uint8_t postureId = getPostureIdByIndex(fg, spriteIndex);
        if (nonMovingCreatureRenderTypeByPosture.size() <= postureId)
        {
            nonMovingCreatureRenderTypeByPosture.resize(postureId + 1);
        }

        if (!nonMovingCreatureRenderTypeByPosture.at(postureId).has_value())
        {
            cacheNonMovingRenderSizes(postureId);
        }

        switch (*nonMovingCreatureRenderTypeByPosture.at(postureId))
        {
            case NonMovingCreatureRenderType::Full:
                break;
            case NonMovingCreatureRenderType::Half:
                // switch (direction)
                // {
                //     case Direction::North:
                //     case Direction::South:
                //     {
                //         auto width = info.window.x1;
                //         info.window.x0 += width / 2;

                //         // Width in unnormalized case
                //         info.window.x1 /= 2;
                //     }
                //     break;
                //     case Direction::East:
                //     case Direction::West:
                //     {
                //         auto height = info.window.y1;
                //         info.window.y0 += height / 2;

                //         // Width in unnormalized case
                //         info.window.y1 /= 2;
                //     }
                //     break;
                // }
                break;
            case NonMovingCreatureRenderType::SingleQuadrant:
            {
                auto width = info.window.x1;
                auto height = info.window.y1;

                info.window.x0 += width / 2;
                // info.window.y0 += height / 2;

                info.window.x1 /= 2;
                info.window.y1 /= 2;
                break;
            }
        }
    }

    return info;
}

const TextureInfo CreatureAppearance::getTextureInfo(uint32_t frameGroupId, Direction direction, TextureInfo::CoordinateType coordinateType) const
{
    auto &fg = this->frameGroup(frameGroupId);
    uint8_t layers = fg.spriteInfo.layers;

    uint32_t spriteIndex = to_underlying(direction) * layers;

    return getTextureInfo(frameGroupId, spriteIndex, coordinateType);
}

TextureAtlas *CreatureAppearance::getTextureAtlas(uint32_t spriteId) const
{
    for (const auto atlas : _atlases)
    {
        if (atlas == nullptr)
        {
            return nullptr;
        }

        if (atlas->firstSpriteId <= spriteId && spriteId <= atlas->lastSpriteId)
        {
            return atlas;
        }
    }

    return Appearances::getTextureAtlas(spriteId);
}

void CreatureAppearance::cacheTextureAtlases()
{
    if (_atlases.front() != nullptr)
    {
        // Atlases already cached
        return;
    }

    for (int i = 0; i < _frameGroups.size(); ++i)
    {
        for (const auto spriteId : _frameGroups.at(i).spriteInfo.spriteIds)
        {
            // Stop if the cache is full
            if (_atlases.back() != nullptr)
            {
                return;
            }
            cacheTextureAtlas(spriteId);
        }
    }
}

void CreatureAppearance::cacheTextureAtlas(uint32_t spriteId)
{
    // If nothing is cached, cache the TextureAtlas for the first sprite ID in the appearance.
    if (_atlases.front() == nullptr)
    {
        uint32_t firstSpriteId = _frameGroups.at(0).getSpriteId(0);
        _atlases.front() = Appearances::getTextureAtlas(firstSpriteId);
    }

    for (int i = 0; i < _atlases.size(); ++i)
    {
        TextureAtlas *&atlas = _atlases[i];
        // End of current cache reached, caching the atlas
        if (atlas == nullptr)
        {
            atlas = Appearances::getTextureAtlas(spriteId);
            return;
        }
        else
        {
            if (atlas->firstSpriteId <= spriteId && spriteId <= atlas->lastSpriteId)
            {
                // The TextureAtlas is already cached
                return;
            }
        }
    }
}

const std::vector<FrameGroup> &CreatureAppearance::frameGroups() const noexcept
{
    return _frameGroups;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>Code for helping with drawing creatures in proper sizes within the UI>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

void CreatureAppearance::cacheNonMovingRenderSizes(uint8_t postureId) const
{
    nonMovingCreatureRenderTypeByPosture.at(postureId) = checkTransparency(postureId);
}

bool isMagenta(int x, int y, int atlasWidth, const std::vector<uint8_t> &pixels)
{
    int i = (atlasWidth - y - 1) * (atlasWidth * 4) + x * 4;
    uint8_t r = pixels.at(i);
    uint8_t g = pixels.at(i + 1);
    uint8_t b = pixels.at(i + 2);

    return r == 255 && g == 0 && b == 255;
}

bool transparentRegion(int fromX, int fromY, int width, int height, int atlasWidth, const std::vector<uint8_t> &pixels)
{
    for (int y = fromY; y < fromY + height; ++y)
    {
        for (int x = fromX; x < fromX + width; ++x)
        {
            if (!isMagenta(x, y, atlasWidth, pixels))
            {
                // VME_LOG_D(std::format("Fail at: {}, {}", x, y));
                return false;
            }
        }
    }

    return true;
}

uint32_t CreatureAppearance::getIndex(const FrameGroup &frameGroup, uint8_t creaturePosture, uint8_t addonType, Direction direction) const
{
    return getIndex(frameGroup, creaturePosture, addonType, to_underlying(direction));
}

uint32_t CreatureAppearance::getIndex(const FrameGroup &frameGroup, uint8_t creaturePosture, uint8_t addonType, uint8_t direction) const
{
    uint8_t directions = frameGroup.spriteInfo.patternWidth;
    uint8_t addons = frameGroup.spriteInfo.patternHeight;
    uint8_t layers = frameGroup.spriteInfo.layers;

    return layers * (directions * (creaturePosture * addons + addonType) + direction);
}

CreatureAppearance::NonMovingCreatureRenderType CreatureAppearance::checkTransparency(uint8_t postureId) const
{
    auto &fg = this->frameGroup(0);
    if (fg.spriteInfo.spriteIds.size() <= to_underlying(Direction::North))
    {
        return NonMovingCreatureRenderType::Full;
    }

    uint8_t postureCount = fg.spriteInfo.patternDepth;

    // Outfit with mount variation (are there other possibilities for patternDepth > 1?)
    if (postureId > 0)
    {
        return NonMovingCreatureRenderType::Full;
    }

    auto spriteId = fg.getSpriteId(getIndex(fg, postureId, 0, 0));
    TextureAtlas *atlas = getTextureAtlas(spriteId);

    const auto &pixels = atlas->getOrCreateTexture().pixels();

    int spriteIndex = spriteId - atlas->firstSpriteId;

    auto topLeftX = atlas->spriteWidth * (spriteIndex % atlas->columns);
    auto topLeftY = atlas->spriteHeight * (spriteIndex / atlas->rows);

    int quadWidth = atlas->spriteWidth / 2;
    int quadHeight = atlas->spriteHeight / 2;

    // Top-left
    if (!transparentRegion(topLeftX, topLeftY, quadWidth, quadHeight, atlas->width, pixels))
    {
        return NonMovingCreatureRenderType::Full;
    }

    // Top-right
    if (!transparentRegion(topLeftX + quadWidth, topLeftY, quadWidth, quadHeight, atlas->width, pixels))
    {
        return NonMovingCreatureRenderType::Half;
    }

    return NonMovingCreatureRenderType::SingleQuadrant;
}

const FrameGroup &CreatureAppearance::frameGroup(size_t index) const
{
    return _frameGroups.at(index);
}