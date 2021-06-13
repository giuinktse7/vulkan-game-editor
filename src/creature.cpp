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
    : creatureType(creatureType), _name("") {}

Creature::Creature(std::string name, const CreatureType &creatureType)
    : creatureType(creatureType), _name(name) {}

Creature::Creature(Creature &&other) noexcept
    : creatureType(other.creatureType),
      selected(other.selected),
      _direction(other._direction),
      _name(std::move(other._name)) {}

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
    newCreature._name = _name;

    return newCreature;
}

void Creature::setDirection(CreatureDirection direction)
{
    _direction = direction;
}

bool Creature::hasName() const noexcept
{
    return _name != "";
}

std::string Creature::name() const noexcept
{
    return _name;
}

const TextureInfo Creature::getTextureInfo() const
{
    return creatureType.getTextureInfo(0, _direction);
}

const TextureInfo CreatureType::getTextureInfo() const
{
    return getTextureInfo(0, CreatureDirection::South);
}

const TextureInfo CreatureType::getTextureInfo(uint32_t frameGroupId, CreatureDirection direction) const
{
    return getTextureInfo(frameGroupId, direction, TextureInfo::CoordinateType::Normalized);
}

const TextureInfo CreatureType::getTextureInfo(uint32_t frameGroupId, CreatureDirection direction, TextureInfo::CoordinateType coordinateType) const
{

    auto &fg = this->frameGroup(frameGroupId);

    uint32_t spriteIndex = std::min<uint32_t>(to_underlying(direction), static_cast<uint32_t>(fg.spriteInfo.spriteIds.size()) - 1);
    auto spriteId = fg.spriteInfo.spriteIds.at(spriteIndex);

    TextureAtlas *atlas = getTextureAtlas(spriteId);

    TextureInfo info;
    info.atlas = atlas;
    info.window = atlas->getTextureWindow(spriteId, coordinateType);

    if (coordinateType == TextureInfo::CoordinateType::Unnormalized)
    {
        if (!nonMovingCreatureRenderType.has_value())
        {
            cacheNonMovingRenderSizes();
        }

        switch (*nonMovingCreatureRenderType)
        {
            case NonMovingCreatureRenderType::Full:
                break;
            case NonMovingCreatureRenderType::Half:
                // switch (direction)
                // {
                //     case CreatureDirection::North:
                //     case CreatureDirection::South:
                //     {
                //         auto width = info.window.x1;
                //         info.window.x0 += width / 2;

                //         // Width in unnormalized case
                //         info.window.x1 /= 2;
                //     }
                //     break;
                //     case CreatureDirection::East:
                //     case CreatureDirection::West:
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

const std::vector<FrameGroup> &CreatureType::frameGroups() const noexcept
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

void CreatureType::cacheNonMovingRenderSizes() const
{
    nonMovingCreatureRenderType = checkTransparency();
}

Pixel getPixel(int x, int y, int atlasWidth, const std::vector<uint8_t> &pixels)
{
    int i = (atlasWidth - y) * (atlasWidth * 4) + x * 4;

    Pixel pixel{};
    pixel.r = pixels.at(i);
    pixel.g = pixels.at(i + 1);
    pixel.b = pixels.at(i + 2);
    pixel.a = pixels.at(i + 3);

    return pixel;
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

CreatureType::NonMovingCreatureRenderType CreatureType::checkTransparency() const
{
    auto &fg = this->frameGroup(0);

    if (fg.spriteInfo.spriteIds.size() <= to_underlying(CreatureDirection::North))
    {
        return NonMovingCreatureRenderType::Full;
    }

    auto spriteId = fg.spriteInfo.spriteIds.at(to_underlying(CreatureDirection::North));
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