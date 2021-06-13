#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "frame_group.h"
#include "graphics/texture_atlas.h"
#include "util.h"

class CreatureAppearance;

namespace tibia::protobuf::appearances
{
    class Appearance;
}

enum class CreatureDirection : uint8_t
{
    North = 0,
    East = 1,
    South = 2,
    West = 3
};

struct Outfit
{
    struct Look
    {
        uint16_t type;
        uint8_t item;
        uint8_t addon;
        uint8_t head;
        uint8_t body;
        uint8_t legs;
        uint8_t feet;
    } look;

    Outfit(uint16_t looktype);
    Outfit(Look look);
};

class CreatureType
{
  public:
    void cacheTextureAtlases();

    CreatureType(std::string id, std::string name, uint16_t looktype);
    CreatureType(std::string id, std::string name, Outfit outfit);

    CreatureType(CreatureType &&other) noexcept;

    inline const std::string &id() const noexcept;
    inline const std::string &name() const noexcept;
    inline uint16_t looktype() const noexcept;

    const FrameGroup &frameGroup(size_t index) const;
    const std::vector<FrameGroup> &frameGroups() const noexcept;
    TextureAtlas *getTextureAtlas(uint32_t spriteId) const;

    const TextureInfo getTextureInfo() const;
    const TextureInfo getTextureInfo(uint32_t frameGroupId, CreatureDirection direction) const;
    const TextureInfo getTextureInfo(uint32_t frameGroupId, CreatureDirection direction, TextureInfo::CoordinateType coordinateType) const;

    CreatureAppearance *appearance = nullptr;

  private:
    Outfit outfit;
    std::string _id;
    std::string _name;

    // bool _npc;
};

class Creatures
{
  public:
    static CreatureType *addCreatureType(std::string id, std::string name, Outfit outfit);

    static inline const CreatureType *creatureType(const std::string &id);
    static const CreatureType *creatureType(uint16_t looktype);

    static bool isValidLooktype(uint16_t looktype);

  private:
    friend class CreatureType;
    Creatures()
    {
        // Empty
    }

    static vme_unordered_map<std::string, std::unique_ptr<CreatureType>> _creatureTypes;
};

class Creature
{
  public:
    Creature(const CreatureType &creatureType);

    Creature(Creature &&other) noexcept;
    // Can not be implemented because creatureType is const.
    Creature &operator=(Creature &&other) = delete;

    static std::optional<Creature> fromOutfitId(uint32_t outfitId);

    Creature deepCopy() const;
    std::string name() const noexcept;
    bool hasName() const noexcept;

    void setDirection(CreatureDirection direction);

    const TextureInfo getTextureInfo() const;

    const CreatureType &creatureType;

    bool selected = false;

  private:
    CreatureDirection _direction = CreatureDirection::South;
};

inline const std::string &CreatureType::id() const noexcept
{
    return _id;
}

inline const std::string &CreatureType::name() const noexcept
{
    return _name;
}

inline uint16_t CreatureType::looktype() const noexcept
{
    return outfit.look.type;
}

inline const CreatureType *Creatures::creatureType(const std::string &id)
{
    auto result = _creatureTypes.find(id);
    return result == _creatureTypes.end() ? nullptr : result->second.get();
}

struct Pixel
{
    int r;
    int g;
    int b;
    int a;
};

inline std::ostream &operator<<(std::ostream &os, const CreatureDirection &direction)
{
    switch (direction)
    {
        case CreatureDirection::North:
            os << "North";
            break;
        case CreatureDirection::East:
            os << "East";
            break;
        case CreatureDirection::South:
            os << "South";
            break;
        case CreatureDirection::West:
            os << "West";
            break;
    }

    return os;
}
inline std::ostream &operator<<(std::ostream &os, const Pixel &pixel)
{
    os << std::format("({}, {}, {}, {})", int(pixel.r), int(pixel.g), int(pixel.b), int(pixel.a));
    return os;
}