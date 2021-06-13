#include "creature.h"

#include "graphics/appearances.h"
#include "logger.h"

vme_unordered_map<std::string, std::unique_ptr<CreatureType>> Creatures::_creatureTypes;

CreatureType *Creatures::addCreatureType(std::string id, std::string name, Outfit outfit)
{
    if (!isValidLooktype(outfit.look.type))
    {
        VME_LOG_ERROR(std::format("Cannot create CreatureType for invalid looktype '{}'", outfit.look.type));
        return nullptr;
    }

    _creatureTypes.emplace(id, std::make_unique<CreatureType>(id, name, outfit));
    return _creatureTypes.at(id).get();
}

bool Creatures::isValidLooktype(uint16_t looktype)
{
    return Appearances::hasCreatureLooktype(looktype);
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

CreatureType::CreatureType(std::string id, std::string name, uint16_t looktype)
    : CreatureType(id, name, Outfit(looktype)) {}

CreatureType::CreatureType(std::string id, std::string name, Outfit outfit)
    : _id(id), _name(name), outfit(outfit), appearance(Appearances::getCreatureAppearance(outfit.look.type))
{
    appearance->cacheTextureAtlases();
}

const FrameGroup &CreatureType::frameGroup(size_t index) const
{
    return appearance->frameGroup(index);
}

const TextureInfo CreatureType::getTextureInfo() const
{
    return getTextureInfo(0, CreatureDirection::South);
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

Outfit::Outfit(uint16_t looktype)
{
    look.type = looktype;
}

Outfit::Outfit(Look look)
    : look(look) {}