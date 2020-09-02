#include "item_type.h"

#include "ecs/ecs.h"
#include "ecs/item_animation.h"

ItemType::~ItemType()
{
  // Empty
}

const uint32_t ItemType::getPatternIndex(const Position &pos) const
{
  const SpriteInfo &spriteInfo = appearance->getSpriteInfo();

  uint32_t width = spriteInfo.patternWidth;
  uint32_t height = spriteInfo.patternHeight;
  uint32_t depth = spriteInfo.patternDepth;

  uint32_t spriteIndex = (pos.x % width) + (pos.y % height) * width + (pos.z % depth) * height * width;

  return spriteIndex;
}

const TextureInfo ItemType::getTextureInfo() const
{
  uint32_t spriteId = appearance->getFirstSpriteId();
  return getTextureInfo(spriteId);
}

const TextureInfo ItemType::getTextureInfo(uint32_t spriteId) const
{
  TextureAtlas *atlas = getTextureAtlas(spriteId);

  float offsetX = 0.5f / atlas->width;
  float offsetY = 0.5f / atlas->height;

  TextureInfo info;
  info.atlas = atlas;
  info.window = atlas->getTextureWindow(spriteId);

  return info;
}

const TextureInfo ItemType::getTextureInfo(const Position &pos) const
{
  if (!appearance->hasFlag(AppearanceFlag::Take) && appearance->hasFlag(AppearanceFlag::Unmove))
  {
    const SpriteInfo &spriteInfo = appearance->getSpriteInfo();

    uint32_t width = spriteInfo.patternWidth;
    uint32_t height = spriteInfo.patternHeight;
    uint32_t depth = spriteInfo.patternDepth;

    uint32_t spriteIndex = (pos.x % width) + (pos.y % height) * width + (pos.z % depth) * height * width;

    uint32_t spriteId = spriteInfo.spriteIds.at(spriteIndex);
    TextureAtlas *atlas = getTextureAtlas(spriteId);

    return TextureInfo{atlas, atlas->getTextureWindow(spriteId)};
  }
  else
  {
    return getTextureInfo();
  }
}

std::vector<TextureAtlas *> ItemType::getTextureAtlases() const
{
  auto comparator = [](TextureAtlas *a, TextureAtlas *b) { return a->sourceFile.compare(b->sourceFile); };
  std::set<TextureAtlas *, decltype(comparator)> textureAtlases(comparator);

  auto &info = this->appearance->getSpriteInfo();

  for (const auto id : info.spriteIds)
    textureAtlases.insert(getTextureAtlas(id));

  return std::vector(textureAtlases.begin(), textureAtlases.end());
}

void ItemType::cacheTextureAtlases()
{
  for (int frameGroup = 0; frameGroup < appearance->frameGroupCount(); ++frameGroup)
  {
    for (const auto spriteId : appearance->getSpriteInfo(frameGroup).spriteIds)
    {
      // Stop if the cache is full
      if (this->atlases.back() != nullptr)
      {
        return;
      }
      cacheTextureAtlas(spriteId);
    }
  }
}

void ItemType::cacheTextureAtlas(uint32_t spriteId)
{
  // If nothing is cached, cache the TextureAtlas for the first sprite ID in the appearance.
  if (this->atlases.front() == nullptr)
    this->atlases.front() = Appearances::getTextureAtlas(this->appearance->getFirstSpriteId());

  for (int i = 0; i < atlases.size(); ++i)
  {
    TextureAtlas *&atlas = this->atlases[i];
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

TextureAtlas *ItemType::getTextureAtlas(uint32_t spriteId) const
{
  for (const auto atlas : atlases)
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

TextureAtlas *ItemType::getFirstTextureAtlas() const
{
  return atlases.front();
}

std::string ItemType::getPluralName() const
{
  if (!pluralName.empty())
  {
    return pluralName;
  }

  if (showCount == 0)
  {
    return name;
  }

  std::string str;
  str.reserve(name.length() + 1);
  str.assign(name);
  str += 's';
  return str;
}

bool ItemType::isGroundTile() const
{
  return group == ITEM_GROUP_GROUND;
}
bool ItemType::isContainer() const
{
  return group == ITEM_GROUP_CONTAINER;
}
bool ItemType::isSplash() const
{
  return group == ITEM_GROUP_SPLASH;
}
bool ItemType::isFluidContainer() const
{
  return group == ITEM_GROUP_FLUID;
}

bool ItemType::isDoor() const
{
  return (type == ITEM_TYPE_DOOR);
}
bool ItemType::isMagicField() const
{
  return (type == ITEM_TYPE_MAGICFIELD);
}
bool ItemType::isTeleport() const
{
  return (type == ITEM_TYPE_TELEPORT);
}
bool ItemType::isKey() const
{
  return (type == ITEM_TYPE_KEY);
}
bool ItemType::isDepot() const
{
  return (type == ITEM_TYPE_DEPOT);
}
bool ItemType::isMailbox() const
{
  return (type == ITEM_TYPE_MAILBOX);
}
bool ItemType::isTrashHolder() const
{
  return (type == ITEM_TYPE_TRASHHOLDER);
}
bool ItemType::isBed() const
{
  return (type == ITEM_TYPE_BED);
}

bool ItemType::isRune() const
{
  return (type == ITEM_TYPE_RUNE);
}
bool ItemType::isPickupable() const
{
  return (allowPickupable || pickupable);
}
bool ItemType::isUseable() const
{
  return (useable);
}
bool ItemType::hasSubType() const
{
  return (isFluidContainer() || isSplash() || stackable || charges != 0);
}

bool ItemType::usesSubType() const
{
  return isStackable() || isSplash() || isFluidContainer();
}

bool ItemType::isStackable() const { return stackable; }