#include "creature.h"

#include "graphics/appearances.h"
#include "logger.h"

#include <set>

vme_unordered_map<std::string, std::unique_ptr<CreatureType>> Creatures::_creatureTypes;

CreatureType *Creatures::addCreatureType(std::string id, std::string name, Outfit outfit)
{
    if (!isValidLooktype(outfit.look.type))
    {
        VME_LOG_ERROR(std::format("Cannot create CreatureType for invalid looktype '{}'", outfit.look.type));
        return nullptr;
    }

    _creatureTypes.emplace(id, std::make_unique<CreatureType>(id, name, outfit));
    auto creatureType = _creatureTypes.at(id).get();

    // std::set<std::string> atlases;

    // if (outfit.look.type == 146)
    // {
    //     auto &fg = creatureType->frameGroup(0);
    //     auto &info = fg.spriteInfo;
    //     for (auto spriteId : fg.spriteInfo.spriteIds)
    //     {
    //         TextureAtlas *atlas = creatureType->getTextureAtlas(spriteId);
    //         atlases.insert(atlas->sourceFile.string());
    //         auto index = atlas->lastSpriteId - spriteId;
    //         VME_LOG(std::format("{}: {}", atlas->sourceFile.string(), 36 - index));
    //     }
    // }

    // for (auto k : atlases)
    // {
    //     VME_LOG(k);
    // }

    auto variationId = outfit.id();

    // TODO Check that the Texture Atlases have not already been supplied with a variant for this outfit.
    // If they have, then the colors will be multiplied more than once resulting in incorrect colors.

    // Layers > 1 means we have a template. If so, instantiate the necessary atlas templates for this creature type
    if (creatureType->frameGroup(0).spriteInfo.layers > 1)
    {
        auto &frameGroup = creatureType->frameGroup(0);

        for (uint8_t postureType = 0; postureType < frameGroup.spriteInfo.patternDepth; ++postureType)
        {
            for (uint8_t addonType = 0; addonType < frameGroup.spriteInfo.patternHeight; ++addonType)
            {
                for (uint8_t direction = 0; direction < frameGroup.spriteInfo.patternWidth; ++direction)
                {
                    uint32_t spriteIndex = creatureType->getIndex(frameGroup, direction, addonType, postureType);

                    uint32_t spriteId = frameGroup.getSpriteId(spriteIndex);
                    uint32_t templateSpriteId = frameGroup.getSpriteId(spriteIndex + 1);

                    TextureAtlas *targetAtlas = creatureType->getTextureAtlas(spriteId);
                    TextureAtlas *templateAtlas = creatureType->getTextureAtlas(templateSpriteId);

                    targetAtlas->overlay(templateAtlas, variationId, templateSpriteId, spriteId, outfit.look);
                }
            }
        }
    }

    return creatureType;
}

uint32_t CreatureType::getIndex(const FrameGroup &frameGroup, uint8_t direction, uint8_t addonType, uint8_t creaturePosture) const
{
    uint8_t directions = frameGroup.spriteInfo.patternWidth;
    uint8_t addons = frameGroup.spriteInfo.patternHeight;
    uint8_t patternDepth = frameGroup.spriteInfo.patternHeight;
    uint8_t layers = frameGroup.spriteInfo.layers;

    return layers * (directions * (creaturePosture * addons + addonType) + direction);
}

bool Creatures::isValidLooktype(uint16_t looktype)
{
    return Appearances::hasCreatureLooktype(looktype);
}

CreatureType::CreatureType(std::string id, std::string name, uint16_t looktype)
    : CreatureType(id, name, Outfit(looktype)) {}

CreatureType::CreatureType(std::string id, std::string name, Outfit outfit)
    : _id(id), _name(name), outfit(outfit), appearance(Appearances::getCreatureAppearance(outfit.look.type))
{
    appearance->cacheTextureAtlases();
}

CreatureType::CreatureType(CreatureType &&other) noexcept
    : appearance(other.appearance),
      outfit(other.outfit),
      _id(other._id),
      _name(other._name) {}

const CreatureType *Creatures::creatureType(uint16_t looktype)
{
    auto found = std::find_if(_creatureTypes.begin(), _creatureTypes.end(), [looktype](const auto &pair) {
        return pair.second->looktype() == looktype;
    });
    if (found != _creatureTypes.end())
    {
        return found->second.get();
    }

    return nullptr;
}

uint16_t CreatureType::getSpriteWidth() const
{
    return appearance->getTextureAtlas(0)->spriteWidth;
}

uint16_t CreatureType::getSpriteHeight() const
{
    return appearance->getTextureAtlas(0)->spriteHeight;
}

uint16_t CreatureType::getAtlasWidth() const
{
    return appearance->getTextureAtlas(0)->width;
}

const FrameGroup &CreatureType::frameGroup(size_t index) const
{
    return appearance->frameGroup(index);
}

const TextureInfo CreatureType::getTextureInfo() const
{
    return getTextureInfo(0, CreatureDirection::South);
}

TextureAtlas *CreatureType::getTextureAtlas(uint32_t spriteId) const
{
    return appearance->getTextureAtlas(spriteId);
}

TextureAtlas *CreatureType::getTextureAtlas(uint32_t frameGroupId, CreatureDirection direction) const
{
    return appearance->getTextureAtlas(getSpriteId(frameGroupId, direction));
}

uint32_t CreatureType::getSpriteIndex(uint32_t frameGroupId, CreatureDirection direction) const
{
    auto &fg = this->frameGroup(frameGroupId);
    uint8_t layers = fg.spriteInfo.layers;

    return to_underlying(direction) * layers;
}

uint32_t CreatureType::getSpriteId(uint32_t frameGroupId, CreatureDirection direction) const
{
    uint32_t spriteIndex = getSpriteIndex(frameGroupId, direction);
    return this->frameGroup(frameGroupId).getSpriteId(spriteIndex);
}

const TextureInfo CreatureType::getTextureInfo(uint32_t frameGroupId, CreatureDirection direction) const
{
    return appearance->getTextureInfo(frameGroupId, direction, TextureInfo::CoordinateType::Normalized);
}

const TextureInfo CreatureType::getTextureInfo(uint32_t frameGroupId, CreatureDirection direction, TextureInfo::CoordinateType coordinateType) const
{
    return appearance->getTextureInfo(frameGroupId, direction, coordinateType);
}

const std::vector<FrameGroup> &CreatureType::frameGroups() const noexcept
{
    return appearance->frameGroups();
}

Creature::Creature(const CreatureType &creatureType)
    : creatureType(creatureType) {}

Creature::Creature(Creature &&other) noexcept
    : creatureType(other.creatureType),
      selected(other.selected),
      _direction(other._direction) {}

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

void Creature::setDirection(CreatureDirection direction)
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

uint32_t CreatureType::outfitId() const noexcept
{
    return outfit.id();
}

bool CreatureType::hasColorVariation() const
{
    return frameGroup(0).spriteInfo.layers > 1;
}
