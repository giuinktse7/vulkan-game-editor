#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include "frame_group.h"
#include "graphics/texture_atlas.h"
#include "outfit.h"
#include "util.h"

class CreatureAppearance;
class ObjectAppearance;
class Item;

namespace tibia::protobuf::appearances
{
    class Appearance;
}

class CreatureType
{
  public:
    CreatureType(std::string id, std::string name, uint16_t looktype);
    CreatureType(std::string id, std::string name, Outfit outfit);

    CreatureType(CreatureType &&other) noexcept;

    inline const std::string &id() const noexcept;
    inline const std::string &name() const noexcept;
    inline uint16_t looktype() const noexcept;

    const FrameGroup &frameGroup(size_t index) const;
    const std::vector<FrameGroup> &frameGroups() const noexcept;
    TextureAtlas *getTextureAtlas(uint32_t spriteId) const;
    TextureAtlas *getTextureAtlas(uint32_t frameGroupId, Direction direction) const;
    uint32_t getSpriteIndex(uint32_t frameGroupId, Direction direction) const;
    uint32_t getSpriteId(uint32_t frameGroupId, Direction direction) const;

    uint16_t getSpriteWidth() const;
    uint16_t getSpriteHeight() const;
    uint16_t getAtlasWidth() const;

    uint32_t getIndex(const FrameGroup &frameGroup, uint8_t creaturePosture, uint8_t addonType, uint8_t direction) const;
    uint32_t getIndex(const FrameGroup &frameGroup, uint8_t creaturePosture, uint8_t addonType, Direction direction) const;

    const TextureInfo getTextureInfo() const;
    const TextureInfo getTextureInfo(uint32_t frameGroupId, Direction direction) const;
    const TextureInfo getTextureInfo(uint32_t frameGroupId, Direction direction, TextureInfo::CoordinateType coordinateType) const;

    const TextureInfo getTextureInfo(uint32_t frameGroupId, int posture, int addonType, Direction direction, TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;

    const Outfit &outfit() const noexcept
    {
        return _outfit;
    }

    bool hasAddon(Outfit::Addon addon) const;
    bool hasMount() const;

    uint16_t mountLooktype() const;

    bool hasColorVariation() const;

    uint32_t outfitId() const noexcept;

  private:
    enum class AppearanceType
    {
        Creature,
        Object
    };

    Outfit _outfit;
    std::string _id;
    std::string _name;

    /**
     * Stores the creature appearance. A creature appearance can be either:
     * 1. A creature-based appearance (CreatureAppearance)
     * 2. An object-based appearance (ObjectAppearance)
     */
    struct Appearance
    {
        Appearance(CreatureAppearance *creatureAppearance);
        Appearance(uint32_t serverId);
        Appearance();

        void cacheTextureAtlases();

        const TextureInfo getTextureInfo() const;
        const TextureInfo getTextureInfo(uint32_t frameGroupId, Direction direction) const;
        const TextureInfo getTextureInfo(uint32_t frameGroupId, Direction direction, TextureInfo::CoordinateType coordinateType) const;
        const TextureInfo getTextureInfo(uint32_t frameGroupId, int posture, int addonType, Direction direction, TextureInfo::CoordinateType coordinateType) const;

        const std::vector<FrameGroup> &frameGroups() const noexcept;

        uint32_t getIndex(const FrameGroup &frameGroup, uint8_t creaturePosture, uint8_t addonType, Direction direction) const;

        TextureAtlas *getTextureAtlas(uint32_t spriteId) const;

        bool isItem() const noexcept;

        Item *asItem() const noexcept;
        CreatureAppearance *asCreatureAppearance() const noexcept;

      private:
        std::variant<std::shared_ptr<Item>, CreatureAppearance *> appearance;
    } appearance;

    // bool _npc;
};

class Creatures
{
  public:
    static CreatureType *addCreatureType(std::string id, std::string name, Outfit outfit);

    static inline CreatureType *creatureType(const std::string &id);
    static const CreatureType *creatureType(uint16_t looktype);

    static bool isValidLooktype(uint16_t looktype);

    static constexpr Direction directions[] = {
        Direction::North,
        Direction::East,
        Direction::South,
        Direction::West,
    };

    static Pixel getColorFromLookupTable(uint8_t looktype);

  private:
    friend class CreatureType;
    Creatures()
    {
        // Empty
    }

    static void createTextureVariation(CreatureType *creatureType, const Outfit &outfit);

    static vme_unordered_map<std::string, std::unique_ptr<CreatureType>> _creatureTypes;

    /**
        For quick lookup by id. Only creaturetypes that have nothing except a looktype are indexed.
    */
    static vme_unordered_map<uint16_t, std::string> _looktypeToIdIndex;

    static constexpr uint32_t TemplateOutfitLookupTable[] = {
        0xFFFFFF,
        0xFFD4BF,
        0xFFE9BF,
        0xFFFFBF,
        0xE9FFBF,
        0xD4FFBF,
        0xBFFFBF,
        0xBFFFD4,
        0xBFFFE9,
        0xBFFFFF,
        0xBFE9FF,
        0xBFD4FF,
        0xBFBFFF,
        0xD4BFFF,
        0xE9BFFF,
        0xFFBFFF,
        0xFFBFE9,
        0xFFBFD4,
        0xFFBFBF,
        0xDADADA,
        0xBF9F8F,
        0xBFAF8F,
        0xBFBF8F,
        0xAFBF8F,
        0x9FBF8F,
        0x8FBF8F,
        0x8FBF9F,
        0x8FBFAF,
        0x8FBFBF,
        0x8FAFBF,
        0x8F9FBF,
        0x8F8FBF,
        0x9F8FBF,
        0xAF8FBF,
        0xBF8FBF,
        0xBF8FAF,
        0xBF8F9F,
        0xBF8F8F,
        0xB6B6B6,
        0xBF7F5F,
        0xBFAF8F,
        0xBFBF5F,
        0x9FBF5F,
        0x7FBF5F,
        0x5FBF5F,
        0x5FBF7F,
        0x5FBF9F,
        0x5FBFBF,
        0x5F9FBF,
        0x5F7FBF,
        0x5F5FBF,
        0x7F5FBF,
        0x9F5FBF,
        0xBF5FBF,
        0xBF5F9F,
        0xBF5F7F,
        0xBF5F5F,
        0x919191,
        0xBF6A3F,
        0xBF943F,
        0xBFBF3F,
        0x94BF3F,
        0x6ABF3F,
        0x3FBF3F,
        0x3FBF6A,
        0x3FBF94,
        0x3FBFBF,
        0x3F94BF,
        0x3F6ABF,
        0x3F3FBF,
        0x6A3FBF,
        0x943FBF,
        0xBF3FBF,
        0xBF3F94,
        0xBF3F6A,
        0xBF3F3F,
        0x6D6D6D,
        0xFF5500,
        0xFFAA00,
        0xFFFF00,
        0xAAFF00,
        0x54FF00,
        0x00FF00,
        0x00FF54,
        0x00FFAA,
        0x00FFFF,
        0x00A9FF,
        0x0055FF,
        0x0000FF,
        0x5500FF,
        0xA900FF,
        0xFE00FF,
        0xFF00AA,
        0xFF0055,
        0xFF0000,
        0x484848,
        0xBF3F00,
        0xBF7F00,
        0xBFBF00,
        0x7FBF00,
        0x3FBF00,
        0x00BF00,
        0x00BF3F,
        0x00BF7F,
        0x00BFBF,
        0x007FBF,
        0x003FBF,
        0x0000BF,
        0x3F00BF,
        0x7F00BF,
        0xBF00BF,
        0xBF007F,
        0xBF003F,
        0xBF0000,
        0x242424,
        0x7F2A00,
        0x7F5500,
        0x7F7F00,
        0x557F00,
        0x2A7F00,
        0x007F00,
        0x007F2A,
        0x007F55,
        0x007F7F,
        0x00547F,
        0x002A7F,
        0x00007F,
        0x2A007F,
        0x54007F,
        0x7F007F,
        0x7F0055,
        0x7F002A,
        0x7F0000,
    };
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

    void setDirection(Direction direction);
    Direction direction() const noexcept;

    int spawnInterval() const noexcept;
    void setSpawnInterval(int spawnInterval);

    const TextureInfo getTextureInfo() const;

    const CreatureType &creatureType;

    bool selected = false;

  private:
    Direction _direction = Direction::South;

    int _spawnInterval;
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
    return _outfit.look.type;
}

inline CreatureType *Creatures::creatureType(const std::string &id)
{
    auto result = _creatureTypes.find(id);
    return result == _creatureTypes.end() ? nullptr : result->second.get();
}

inline std::ostream &operator<<(std::ostream &os, const Direction &direction)
{
    switch (direction)
    {
        case Direction::North:
            os << "North";
            break;
        case Direction::East:
            os << "East";
            break;
        case Direction::South:
            os << "South";
            break;
        case Direction::West:
            os << "West";
            break;
    }

    return os;
}
inline std::ostream &operator<<(std::ostream &os, const Pixel &pixel)
{
    os << std::format("({}, {}, {}, {})", int(pixel.r()), int(pixel.g()), int(pixel.b()), int(pixel.a()));
    return os;
}