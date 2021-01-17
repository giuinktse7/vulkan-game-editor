#include "creature.h"

#include "graphics/appearances.h"

vme_unordered_map<uint32_t, CreatureType> Creatures::_creatureTypes;

CreatureType::CreatureType(uint32_t id, std::vector<FrameGroup> &&frameGroups)
    : _frameGroups(std::move(frameGroups)), _id(id) {}

void Creatures::addCreatureType(CreatureType &&creatureType)
{
    uint32_t id = creatureType.id();

    _creatureTypes.emplace(id, std::move(creatureType));
    _creatureTypes.at(id).cacheTextureAtlases();
}

const FrameGroup &CreatureType::frameGroup(size_t index) const
{
    return _frameGroups.at(index);
}

Creature::Creature(const CreatureType &creatureType)
    : creatureType(creatureType)
{
}

Creature::Creature(Creature &&other) noexcept
    : creatureType(other.creatureType),
      _direction(other._direction)
{
}

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

void Creature::setDirection(Creature::Direction direction)
{
    _direction = direction;
}

const TextureInfo Creature::getTextureInfo() const
{
    auto &frameGroup = creatureType.frameGroup(0);

    uint32_t spriteIndex = std::min<uint32_t>(to_underlying(_direction), static_cast<uint32_t>(frameGroup.spriteInfo.spriteIds.size()) - 1);
    auto spriteId = frameGroup.spriteInfo.spriteIds.at(spriteIndex);

    TextureAtlas *atlas = creatureType.getTextureAtlas(spriteId);

    TextureInfo info;
    info.atlas = atlas;
    info.window = atlas->getTextureWindow(spriteId);

    return info;
}

TextureAtlas *CreatureType::getTextureAtlas(uint32_t spriteId) const
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

void CreatureType::cacheTextureAtlases()
{
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

void CreatureType::cacheTextureAtlas(uint32_t spriteId)
{
    // If nothing is cached, cache the TextureAtlas for the first sprite ID in the appearance.
    if (_atlases.front() == nullptr)
    {
        uint32_t firstSpriteId = _frameGroups.at(0).spriteInfo.spriteIds.at(0);
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