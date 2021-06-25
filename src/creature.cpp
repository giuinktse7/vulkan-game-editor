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

    auto variationId = outfit.id();

    // TODO Check that the Texture Atlases have not already been supplied with a variant for this outfit.
    // If they have, then the colors will be multiplied more than once resulting in incorrect colors.

    // Layers > 1 means we have a template. If so, instantiate the necessary atlas templates for this creature type
    if (creatureType->frameGroup(0).spriteInfo.layers > 1)
    {
        std::vector<Texture *> textures;

        auto &frameGroup = creatureType->frameGroup(0);
        uint8_t postureCount = frameGroup.spriteInfo.patternDepth;
        uint8_t addonCount = frameGroup.spriteInfo.patternHeight; // Includes "no addon (base outfit)"
        uint8_t directionCount = frameGroup.spriteInfo.patternWidth;

        for (uint8_t postureType = 0; postureType < postureCount; ++postureType)
        {
            for (uint8_t addonType = 0; addonType < addonCount; ++addonType)
            {
                for (uint8_t direction = 0; direction < directionCount; ++direction)
                {
                    uint32_t spriteIndex = creatureType->getIndex(frameGroup, direction, addonType, postureType);

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
    return getTextureInfo(0, Direction::South);
}

TextureAtlas *CreatureType::getTextureAtlas(uint32_t spriteId) const
{
    return appearance->getTextureAtlas(spriteId);
}

TextureAtlas *CreatureType::getTextureAtlas(uint32_t frameGroupId, Direction direction) const
{
    return appearance->getTextureAtlas(getSpriteId(frameGroupId, direction));
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
    return appearance->getTextureInfo(frameGroupId, direction, TextureInfo::CoordinateType::Normalized);
}

const TextureInfo CreatureType::getTextureInfo(uint32_t frameGroupId, Direction direction, TextureInfo::CoordinateType coordinateType) const
{
    return appearance->getTextureInfo(frameGroupId, direction, coordinateType);
}

const std::vector<FrameGroup> &CreatureType::frameGroups() const noexcept
{
    return appearance->frameGroups();
}

uint32_t CreatureType::outfitId() const noexcept
{
    return outfit.id();
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