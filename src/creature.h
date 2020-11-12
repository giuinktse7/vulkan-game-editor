#pragma once

#include <vector>

#include "graphics/texture_atlas.h"
#include "util.h"

namespace tibia::protobuf::appearances
{
  class Appearance;
}

struct FrameGroup;
class CreatureType;

class Creatures
{
public:
  static void addCreatureType(const tibia::protobuf::appearances::Appearance &appearance);
  static inline const CreatureType *creatureType(uint32_t id);

private:
  friend class CreatureType;
  Creatures()
  {
    // Empty
  }

  static vme_unordered_map<uint32_t, CreatureType> _creatureTypes;
};

class CreatureType
{
public:
  CreatureType(const tibia::protobuf::appearances::Appearance &appearance);

  void cacheTextureAtlases();

  inline uint32_t id() const noexcept;

  const FrameGroup &frameGroup(size_t index) const;
  inline const std::vector<FrameGroup> &frameGroups() const noexcept;
  TextureAtlas *getTextureAtlas(uint32_t spriteId) const;

private:
  void cacheTextureAtlas(uint32_t spriteId);

  static constexpr size_t CachedTextureAtlasAmount = 5;

  std::array<TextureAtlas *, CachedTextureAtlasAmount> _atlases = {};
  std::vector<FrameGroup> _frameGroups;
  uint32_t _id;

  bool _npc;
};

class Creature
{
public:
  enum class Direction : uint8_t
  {
    North = 0,
    East = 1,
    South = 2,
    West = 3
  };

  Creature(const CreatureType &creatureType);

  Creature(Creature &&other) noexcept;
  // Can not be implemented because creatureType is const.
  Creature &operator=(Creature &&other) = delete;

  static std::optional<Creature> fromOutfitId(uint32_t outfitId);

  Creature deepCopy() const;

  void setDirection(Direction direction);

  const TextureInfo getTextureInfo() const;

  const CreatureType &creatureType;

  bool selected = false;

private:
  Direction _direction = Direction::North;
};

inline const std::vector<FrameGroup> &CreatureType::frameGroups() const noexcept
{
  return _frameGroups;
}

inline uint32_t CreatureType::id() const noexcept
{
  return _id;
}

inline const CreatureType *Creatures::creatureType(uint32_t id)
{
  auto result = _creatureTypes.find(id);
  return result == _creatureTypes.end() ? nullptr : &result->second;
}