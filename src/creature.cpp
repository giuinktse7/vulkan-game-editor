#include "creature.h"

#include "config.h"
#include "graphics/appearances.h"
#include "item.h"
#include "items.h"
#include "logger.h"

#include <set>

vme_unordered_map<std::string, std::unique_ptr<CreatureType>> Creatures::_creatureTypes;
vme_unordered_map<uint16_t, std::string> Creatures::_looktypeToIdIndex;

CreatureType *Creatures::addCreatureType(std::string id, std::string name, Outfit outfit)
{
    // Ensure we do not create a creature type with an invalid outfit
    if (outfit.isItem())
    {
        if (!Items::items.validItemType(outfit.look.item))
        {
            VME_LOG_ERROR(std::format("Cannot create CreatureType for outfit with server ID: '{}' because the serverID '{}' does not exist.", outfit.look.item, outfit.look.item));
            return nullptr;
        }
    }
    else
    {
        if (!isValidLooktype(outfit.look.type))
        {
            VME_LOG_ERROR(std::format("Cannot create CreatureType for invalid looktype '{}'", outfit.look.type));
            return nullptr;
        }
    }

    if (creatureType(id))
    {
        VME_LOG_ERROR(std::format("A creature type with id '{}' already exists."));
        return nullptr;
    }

    // Add creature type for the mount if it doesn't exist yet
    if (outfit.look.mount != 0 && creatureType(outfit.look.mount) == nullptr)
    {
        std::string mountId = std::format("mount_looktype_{}", outfit.look.mount);
        std::string mountName = std::format("Mount (looktype {})", outfit.look.mount);
        Creatures::addCreatureType(mountId, mountName, Outfit(outfit.look.mount));
    }

    // Index it by looktype if it only consists of a looktype and nothing else (no colors/mount/addons)
    if (outfit.isOnlyLooktype())
    {
        _looktypeToIdIndex.emplace(outfit.look.type, id);
    }

    _creatureTypes.emplace(id, std::make_unique<CreatureType>(id, name, outfit));
    auto creatureType = _creatureTypes.at(id).get();

    // Layers > 1 means we have a template. If so, instantiate the necessary atlas templates for this creature type
    bool multiLayer = creatureType->frameGroup(0).spriteInfo.layers > 1;
    if (multiLayer)
    {
        bool needsTemplate = creatureType->outfitId() != 0;
        if (needsTemplate)
            createTextureVariation(creatureType, outfit);
    }

    return creatureType;
}

void Creatures::createTextureVariation(CreatureType *creatureType, const Outfit &outfit)
{
    std::vector<Texture *> textures;

    auto &frameGroup = creatureType->frameGroup(0);
    uint8_t postureCount = frameGroup.spriteInfo.patternDepth;
    uint8_t addonCount = frameGroup.spriteInfo.patternHeight; // Includes "no addon (base outfit)"
    uint8_t directionCount = frameGroup.spriteInfo.patternWidth;

    auto variationId = outfit.id();

    for (uint8_t postureType = 0; postureType < postureCount; ++postureType)
    {
        for (uint8_t addonType = 0; addonType < addonCount; ++addonType)
        {
            for (uint8_t direction = 0; direction < directionCount; ++direction)
            {
                uint32_t spriteIndex = creatureType->getIndex(frameGroup, postureType, addonType, direction);

                uint32_t spriteId = frameGroup.getSpriteId(spriteIndex);
                uint32_t templateSpriteId = frameGroup.getSpriteId(spriteIndex + 1);

                TextureAtlas *targetAtlas = creatureType->getTextureAtlas(spriteId);
                auto &targetTexture = targetAtlas->getVariation(variationId)->texture;

                if (targetTexture.finalized())
                {
                    continue;
                }

                // Add texture to the list of textures to be finalized once we have finished with all the overlays
                auto found = std::find_if(textures.begin(), textures.end(), [&targetTexture](const Texture *texture) {
                    return texture->id() == targetTexture.id();
                });
                if (found == textures.end())
                {
                    textures.push_back(&targetTexture);
                }

                TextureAtlas *templateAtlas = creatureType->getTextureAtlas(templateSpriteId);

                targetAtlas->overlay(templateAtlas, variationId, templateSpriteId, spriteId, outfit.look);
            }
        }
    }

    for (Texture *texture : textures)
    {
        texture->finalize();
    }
}

uint32_t CreatureType::getIndex(const FrameGroup &frameGroup, uint8_t creaturePosture, uint8_t addonType, Direction direction) const
{
    return getIndex(frameGroup, creaturePosture, addonType, to_underlying(direction));
}

uint32_t CreatureType::getIndex(const FrameGroup &frameGroup, uint8_t creaturePosture, uint8_t addonType, uint8_t direction) const
{
    uint8_t directions = frameGroup.spriteInfo.patternWidth;
    uint8_t addons = frameGroup.spriteInfo.patternHeight;
    uint8_t layers = frameGroup.spriteInfo.layers;

    return layers * (directions * (creaturePosture * addons + addonType) + direction);
}

bool CreatureType::hasAddon(Outfit::Addon addon) const
{
    return (_outfit.look.addon & addon) != Outfit::Addon::None;
}

bool CreatureType::hasMount() const
{
    return _outfit.look.mount != 0;
}

uint16_t CreatureType::mountLooktype() const
{
    return _outfit.look.mount;
}

bool Creatures::isValidLooktype(uint16_t looktype)
{
    return Appearances::hasCreatureLooktype(looktype);
}

Pixel Creatures::getColorFromLookupTable(uint8_t look)
{
    uint8_t r = (TemplateOutfitLookupTable[look] & 0xFF0000) >> 16;
    uint8_t g = (TemplateOutfitLookupTable[look] & 0xFF00) >> 8;
    uint8_t b = (TemplateOutfitLookupTable[look] & 0xFF);

    return {r, g, b, 255};
}

CreatureType::CreatureType(std::string id, std::string name, uint16_t looktype)
    : CreatureType(id, name, Outfit(looktype)) {}

CreatureType::CreatureType(std::string id, std::string name, Outfit outfit)
    : _id(id), _name(name), _outfit(outfit)
{
    if (_outfit.isItem())
    {
        appearance = Appearance(_outfit.look.item);
    }
    else
    {
        appearance = Appearance(Appearances::getCreatureAppearance(_outfit.look.type));
    }

    appearance.cacheTextureAtlases();
}

CreatureType::CreatureType(CreatureType &&other) noexcept
    : appearance(other.appearance),
      _outfit(other._outfit),
      _id(other._id),
      _name(other._name) {}

const CreatureType *Creatures::creatureType(uint16_t looktype)
{
    auto id = _looktypeToIdIndex.find(looktype);
    if (id == _looktypeToIdIndex.end())
    {
        return nullptr;
    }

    return creatureType(id.value());
    // auto found = std::find_if(_creatureTypes.begin(), _creatureTypes.end(), [looktype](const auto &pair) {
    //     return pair.second->looktype() == looktype;
    // });
    // if (found != _creatureTypes.end())
    // {
    //     return found->second.get();
    // }

    // return nullptr;
}

uint16_t CreatureType::getSpriteWidth() const
{
    return appearance.getTextureAtlas(0)->spriteWidth;
}

uint16_t CreatureType::getSpriteHeight() const
{
    return appearance.getTextureAtlas(0)->spriteHeight;
}

uint16_t CreatureType::getAtlasWidth() const
{
    return appearance.getTextureAtlas(0)->width;
}

const FrameGroup &CreatureType::frameGroup(size_t index) const
{
    return appearance.frameGroups().at(index);
}

const TextureInfo CreatureType::getTextureInfo() const
{
    return getTextureInfo(0, Direction::South);
}

TextureAtlas *CreatureType::getTextureAtlas(uint32_t spriteId) const
{
    return appearance.getTextureAtlas(spriteId);
}

TextureAtlas *CreatureType::getTextureAtlas(uint32_t frameGroupId, Direction direction) const
{
    return appearance.getTextureAtlas(getSpriteId(frameGroupId, direction));
}

uint32_t CreatureType::getSpriteIndex(uint32_t frameGroupId, Direction direction) const
{
    auto &fg = this->frameGroup(frameGroupId);
    uint8_t layers = fg.spriteInfo.layers;

    return to_underlying(direction) * layers;
}

uint32_t CreatureType::getSpriteId(uint32_t frameGroupId, Direction direction) const
{
    uint32_t spriteIndex = getSpriteIndex(frameGroupId, direction);
    return this->frameGroup(frameGroupId).getSpriteId(spriteIndex);
}

const TextureInfo CreatureType::getTextureInfo(uint32_t frameGroupId, Direction direction) const
{
    return appearance.getTextureInfo(frameGroupId, direction, TextureInfo::CoordinateType::Normalized);
}

const TextureInfo CreatureType::getTextureInfo(uint32_t frameGroupId, Direction direction, TextureInfo::CoordinateType coordinateType) const
{
    return appearance.getTextureInfo(frameGroupId, direction, coordinateType);
}

const TextureInfo CreatureType::getTextureInfo(uint32_t frameGroupId, int posture, int addonType, Direction direction, TextureInfo::CoordinateType coordinateType) const
{
    return appearance.getTextureInfo(frameGroupId, posture, addonType, direction, coordinateType);
}

const std::vector<FrameGroup> &CreatureType::frameGroups() const noexcept
{
    return appearance.frameGroups();
}

uint32_t CreatureType::outfitId() const noexcept
{
    return _outfit.id();
}

bool CreatureType::hasColorVariation() const
{
    return frameGroup(0).spriteInfo.layers > 1;
}

//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>>>Creature>>>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>

Creature::Creature(const CreatureType &creatureType)
    : creatureType(creatureType), _spawnInterval(Settings::DEFAULT_CREATURE_SPAWN_INTERVAL) {}

Creature::Creature(Creature &&other) noexcept
    : creatureType(other.creatureType),
      selected(other.selected),
      _direction(other._direction),
      _spawnInterval(other._spawnInterval) {}

std::optional<Creature> Creature::fromOutfitId(uint32_t outfitId)
{
    auto creatureType = Creatures::creatureType(outfitId);
    return creatureType ? std::optional<Creature>(Creature(*creatureType)) : std::nullopt;
}

Creature Creature::deepCopy() const
{
    Creature newCreature(creatureType);
    newCreature.selected = selected;
    newCreature._direction = _direction;

    return newCreature;
}

void Creature::setDirection(Direction direction)
{
    _direction = direction;
}

bool Creature::hasName() const noexcept
{
    return creatureType.name() != "";
}

std::string Creature::name() const noexcept
{
    return creatureType.name();
}

const TextureInfo Creature::getTextureInfo() const
{
    return creatureType.getTextureInfo(0, _direction);
}

Direction Creature::direction() const noexcept
{
    return _direction;
}

int Creature::spawnInterval() const noexcept
{
    return _spawnInterval;
}

void Creature::setSpawnInterval(int spawnInterval)
{
    _spawnInterval = spawnInterval;
}

CreatureType::Appearance::Appearance(CreatureAppearance *creatureAppearance)
    : appearance(creatureAppearance) {}

CreatureType::Appearance::Appearance(uint32_t serverId)
    : appearance(std::make_shared<Item>(serverId)) {}

void CreatureType::Appearance::cacheTextureAtlases()
{
    if (!isItem())
    {
        asCreatureAppearance()->cacheTextureAtlases();
    }
}

CreatureType::Appearance::Appearance()
    : appearance(nullptr) {}

const TextureInfo CreatureType::Appearance::getTextureInfo() const
{
    return getTextureInfo(0, Direction::South);
}

const TextureInfo CreatureType::Appearance::getTextureInfo(uint32_t frameGroupId, Direction direction) const
{
    return getTextureInfo(frameGroupId, direction, TextureInfo::CoordinateType::Normalized);
}

const TextureInfo CreatureType::Appearance::getTextureInfo(uint32_t frameGroupId, Direction direction, TextureInfo::CoordinateType coordinateType) const
{
    return isItem() ? asItem()->getTextureInfo(PositionConstants::Zero, coordinateType)
                    : asCreatureAppearance()->getTextureInfo(frameGroupId, direction, coordinateType);
}

const TextureInfo CreatureType::Appearance::getTextureInfo(uint32_t frameGroupId, int posture, int addonType, Direction direction, TextureInfo::CoordinateType coordinateType) const
{
    if (isItem())
    {
        return asItem()->getTextureInfo(PositionConstants::Zero, coordinateType);
    }
    else
    {
        auto &frameGroup = frameGroups().at(frameGroupId);
        uint32_t index = getIndex(frameGroup, posture, addonType, direction);
        return asCreatureAppearance()->getTextureInfo(frameGroupId, index, coordinateType);
    }
}

bool CreatureType::Appearance::isItem() const noexcept
{
    return std::holds_alternative<std::shared_ptr<Item>>(appearance);
}

Item *CreatureType::Appearance::asItem() const noexcept
{
    return std::get<std::shared_ptr<Item>>(appearance).get();
}

CreatureAppearance *CreatureType::Appearance::asCreatureAppearance() const noexcept
{
    return std::get<CreatureAppearance *>(appearance);
}

uint32_t CreatureType::Appearance::getIndex(const FrameGroup &frameGroup, uint8_t creaturePosture, uint8_t addonType, Direction direction) const
{
    if (isItem())
    {
        return 0;
    }
    else
    {
        uint8_t directions = frameGroup.spriteInfo.patternWidth;
        uint8_t addons = frameGroup.spriteInfo.patternHeight;
        uint8_t layers = frameGroup.spriteInfo.layers;

        return layers * (directions * (creaturePosture * addons + addonType) + to_underlying(direction));
    }
}

TextureAtlas *CreatureType::Appearance::getTextureAtlas(uint32_t spriteId) const
{
    return isItem() ? asItem()->itemType->getTextureAtlas(spriteId)
                    : asCreatureAppearance()->getTextureAtlas(spriteId);
}

const std::vector<FrameGroup> &CreatureType::Appearance::frameGroups() const noexcept
{
    return isItem() ? asItem()->itemType->frameGroups()
                    : asCreatureAppearance()->frameGroups();
}