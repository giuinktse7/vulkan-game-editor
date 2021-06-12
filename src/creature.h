#pragma once

#include <vector>

#include "frame_group.h"
#include "graphics/texture_atlas.h"
#include "util.h"
#include <sstream>

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

class CreatureType
{
  public:
    CreatureType(uint32_t id, std::vector<FrameGroup> &&frameGroups);

    void cacheTextureAtlases();

    inline uint32_t id() const noexcept;
    inline uint32_t looktype() const noexcept;

    const FrameGroup &frameGroup(size_t index) const;
    const std::vector<FrameGroup> &frameGroups() const noexcept;
    TextureAtlas *getTextureAtlas(uint32_t spriteId) const;

    const TextureInfo getTextureInfo() const;
    const TextureInfo getTextureInfo(uint32_t frameGroupId, CreatureDirection direction) const;
    const TextureInfo getTextureInfo(uint32_t frameGroupId, CreatureDirection direction, TextureInfo::CoordinateType coordinateType) const;

  private:
    void cacheTextureAtlas(uint32_t spriteId);

    /**
     * Used to render the creature with the correct size in the UI windows
     */
    enum class NonMovingCreatureRenderType
    {
        Full,
        Half,
        SingleQuadrant
    };

    /**
     * Checks transparency for quadrants in the sprite. Might help with rendering down-scaled (e.g. 64x32 -> 32x32)
     * images in a better way in the UI.
     */
    NonMovingCreatureRenderType checkTransparency() const;

    static constexpr size_t CachedTextureAtlasAmount = 5;

    std::array<TextureAtlas *, CachedTextureAtlasAmount> _atlases = {};
    std::vector<FrameGroup> _frameGroups;
    uint32_t _id;

    mutable std::optional<NonMovingCreatureRenderType> nonMovingCreatureRenderType;

    void cacheNonMovingRenderSizes() const;

    // bool _npc;
};

class Creatures
{
  public:
    static void addCreatureType(CreatureType &&creatureType);
    static inline const CreatureType *creatureType(uint32_t id);

  private:
    friend class CreatureType;
    Creatures()
    {
        // Empty
    }

    static vme_unordered_map<uint32_t, CreatureType> _creatureTypes;
};

class Creature
{
  public:
    Creature(const CreatureType &creatureType);
    Creature(std::string name, const CreatureType &creatureType);

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
    std::string _name;
};

inline uint32_t CreatureType::id() const noexcept
{
    return _id;
}

inline uint32_t CreatureType::looktype() const noexcept
{
    return _id;
}

inline const CreatureType *Creatures::creatureType(uint32_t id)
{
    auto result = _creatureTypes.find(id);
    return result == _creatureTypes.end() ? nullptr : &result->second;
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