#include "item_type.h"

#include "brushes/border_brush.h"
#include "brushes/brush.h"
#include "brushes/ground_brush.h"
#include "graphics/appearances.h"

const uint32_t ItemType::getPatternIndex(const Position &pos) const
{
    const SpriteInfo &spriteInfo = appearance->getSpriteInfo();
    if (spriteInfo.patternSize == 1 || isStackable())
        return 0;

    uint32_t width = spriteInfo.patternWidth;
    uint32_t height = spriteInfo.patternHeight;
    uint32_t depth = spriteInfo.patternDepth;

    uint32_t spriteIndex = (pos.x % width) + (pos.y % height) * width + (pos.z % depth) * height * width;

    return spriteIndex;
}

uint8_t ItemType::getFluidPatternOffset(FluidType fluidType) const
{
    switch (fluidType)
    {
        case FluidType::None:
        case FluidType::Ink:
            return 0;
        case FluidType::Water:
            return 1;
        case FluidType::Blood:
        case FluidType::Life:
        case FluidType::Lava:
            return 2;
        case FluidType::Beer:
        case FluidType::Mud:
        case FluidType::Oil:
        case FluidType::Rum:
        case FluidType::Tea:
        case FluidType::Mead:
            return 3;
        case FluidType::Slime:
        case FluidType::Swamp:
            return 4;
        case FluidType::Lemonade:
        case FluidType::Urine:
        case FluidType::FruitJuice:
            return 5;
        case FluidType::Milk:
        case FluidType::CoconutMilk:
            return 6;
        case FluidType::Mana:
        case FluidType::Wine:
            return 7;
        default:
            return 0;
    }
}

const uint32_t ItemType::getPatternIndexForSubtype(uint8_t subtype) const
{
    DEBUG_ASSERT(usesSubType(), "Invalid call to getPatternIndexForSubtype: the ItemType does not use subtype.");
    DEBUG_ASSERT((!isStackable() || subtype > 0) && subtype < std::numeric_limits<uint8_t>::max(), std::format("Invalid subtype {}. Subtype Must be in [1, 255].", int(subtype)));

    // TODO Handle charges if necessary
    if (isSplash() || isFluidContainer())
    {
        // TODO Handle rotation of fluid containers
        uint8_t fluidPatternOffset = getFluidPatternOffset(static_cast<FluidType>(subtype));
        uint8_t patternIndex = std::min(static_cast<uint8_t>(appearance->spriteCount(0) - 1), fluidPatternOffset);

        return patternIndex;
    }
    else if (stackable)
    {
        // Amount of sprites for different subtypes
        uint8_t stackSpriteCount = appearance->getSpriteInfo().patternSize;
        if (stackSpriteCount == 1)
            return 0;

        if (subtype <= 5)
            return subtype - 1;
        else if (subtype < 10)
            return to_underlying(StackSizeOffset::Five);
        else if (subtype < 25)
            return to_underlying(StackSizeOffset::Ten);
        else if (subtype < 50)
            return to_underlying(StackSizeOffset::TwentyFive);
        else
            return to_underlying(StackSizeOffset::Fifty);
    }
    else
    {
        return 0;
    }
}

uint32_t ItemType::getSpriteId(const Position &pos) const
{
    uint32_t spriteIndex = usesSubType() ? 0 : getPatternIndex(pos);
    return appearance->getSpriteInfo().spriteIds.at(spriteIndex);
}

const TextureInfo ItemType::getTextureInfo(TextureInfo::CoordinateType coordinateType) const
{
    uint32_t spriteId = appearance->getFirstSpriteId();
    return getTextureInfo(spriteId, coordinateType);
}

const TextureInfo ItemType::getTextureInfoTopLeftQuadrant(uint32_t spriteId) const
{
    auto atlas = getTextureAtlas(spriteId);

    return TextureInfo{atlas, atlas->getTextureWindowTopLeft(spriteId)};
}
const std::pair<TextureInfo, TextureInfo> ItemType::getTextureInfoTopLeftBottomRightQuadrant(uint32_t spriteId) const
{
    auto atlas = getTextureAtlas(spriteId);
    const auto [topLeftTextureWindow, bottomRightTextureWindow] = atlas->getTextureWindowTopLeftBottomRight(spriteId);

    return std::pair{
        TextureInfo{atlas, topLeftTextureWindow},
        TextureInfo{atlas, bottomRightTextureWindow}};
}

const std::tuple<TextureInfo, TextureInfo, TextureInfo> ItemType::getTextureInfoTopRightBottomRightBottomLeftQuadrant(uint32_t spriteId) const
{
    auto atlas = getTextureAtlas(spriteId);
    const auto [topRightTextureWindow, bottomRightTextureWindow, bottomLeftTextureWindow] = atlas->getTextureWindowTopRightBottomRightBottomLeft(spriteId);

    return std::tuple{
        TextureInfo{atlas, topRightTextureWindow},
        TextureInfo{atlas, bottomRightTextureWindow},
        TextureInfo{atlas, bottomLeftTextureWindow}};
}

const TextureInfo ItemType::getTextureInfo(const Position &pos, TextureInfo::CoordinateType coordinateType) const
{
    uint32_t spriteId = getSpriteId(pos);

    return getTextureInfo(spriteId, coordinateType);
}

const TextureInfo ItemType::getTextureInfo(uint32_t spriteId, TextureInfo::CoordinateType coordinateType) const
{
    TextureInfo info;
    info.atlas = getTextureAtlas(spriteId);
    info.window = info.atlas->getTextureWindow(spriteId, coordinateType);

    return info;
}

const TextureInfo ItemType::getTextureInfoForSubtype(uint8_t subtype, TextureInfo::CoordinateType coordinateType) const
{
    const SpriteInfo &spriteInfo = appearance->getSpriteInfo();

    uint32_t spriteIndex = getPatternIndexForSubtype(subtype);
    uint32_t spriteId = spriteInfo.spriteIds.at(spriteIndex);

    return getTextureInfo(spriteId, coordinateType);
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
    appearance->cacheTextureAtlases();
}

const std::vector<FrameGroup> &ItemType::frameGroups() const noexcept
{
    return appearance->frameGroups();
}

TextureAtlas *ItemType::getTextureAtlas(uint32_t spriteId) const
{
    for (const auto atlas : appearance->atlases())
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

TextureAtlas *ItemType::getFirstTextureAtlas() const noexcept
{
    return appearance->atlases().front();
}

std::vector<const TextureAtlas *> ItemType::atlases() const
{
    std::vector<const TextureAtlas *> result;
    for (const auto atlas : appearance->atlases())
    {
        if (atlas == nullptr)
            return result;

        result.emplace_back(atlas);
    }

    return result;
}

const SpriteInfo &ItemType::getSpriteInfo(size_t frameGroup) const
{
    return appearance->getSpriteInfo(frameGroup);
}

const SpriteInfo &ItemType::getSpriteInfo() const
{
    return appearance->getSpriteInfo();
}

uint32_t ItemType::speed() const noexcept
{
    return isGround() ? appearance->flagData.groundSpeed : 0;
}

ItemSlot ItemType::inventorySlot() const noexcept
{
    return appearance->flagData.itemSlot;
}

bool ItemType::hasFlag(AppearanceFlag flag) const noexcept
{
    return appearance->hasFlag(flag);
}

bool ItemType::hasAnimation() const noexcept
{
    return appearance->getSpriteInfo().hasAnimation();
}

SpriteAnimation *ItemType::animation() const noexcept
{
    return appearance->getSpriteInfo().animation();
}

int ItemType::getElevation() const noexcept
{
    return appearance->flagData.elevation;
}

bool ItemType::hasElevation() const noexcept
{
    return appearance->hasFlag(AppearanceFlag::Height);
}

bool ItemType::alwaysBottomOfTile() const noexcept
{
    return appearance->hasFlag(AppearanceFlag::Top);
}

std::string ItemType::getPluralName() const
{
    if (!pluralName.empty())
    {
        return pluralName;
    }

    if (!showCount)
    {
        return _name;
    }

    std::string str;
    str.reserve(_name.length() + 1);
    str.assign(_name);
    str += 's';
    return str;
}

uint32_t ItemType::clientId() const noexcept
{
    return appearance->clientId;
}

void ItemType::setName(std::string name) noexcept
{
    _name = name;
}

const std::string &ItemType::name() const noexcept
{
    return _name;
}

bool ItemType::isGround() const noexcept
{
    return appearance->hasFlag(AppearanceFlag::Ground);
}

bool ItemType::isContainer() const noexcept
{
    return group == ItemType::Group::Container || appearance->hasFlag(AppearanceFlag::Container);
}

bool ItemType::isBlocking() const noexcept
{
    return appearance->hasFlag(AppearanceFlag::Unpass) || appearance->hasFlag(AppearanceFlag::Unsight);
}

bool ItemType::isSplash() const noexcept
{
    return group == ItemType::Group::Splash;
}

bool ItemType::isChargeable() const noexcept
{
    return group == ItemType::Group::Charges;
}

bool ItemType::isFluidContainer() const noexcept
{
    return group == ItemType::Group::Fluid;
}

bool ItemType::isCorpse() const noexcept
{
    return appearance->hasFlag(AppearanceFlag::Corpse);
}

bool ItemType::isWriteable() const noexcept
{
    return appearance->hasFlag(AppearanceFlag::Write) || appearance->hasFlag(AppearanceFlag::WriteOnce);
}

bool ItemType::isDoor() const noexcept
{
    return (type == ItemTypes_t::Door);
}

bool ItemType::isMagicField() const noexcept
{
    return (type == ItemTypes_t::MagicField);
}

bool ItemType::isTeleport() const noexcept
{
    return (type == ItemTypes_t::Teleport);
}

bool ItemType::isKey() const noexcept
{
    return (type == ItemTypes_t::Key);
}

bool ItemType::isDepot() const noexcept
{
    return (type == ItemTypes_t::Depot);
}
bool ItemType::isMailbox() const noexcept
{
    return (type == ItemTypes_t::Mailbox);
}

bool ItemType::isTrashHolder() const noexcept
{
    return (type == ItemTypes_t::TrashHolder);
}

bool ItemType::isBed() const noexcept
{
    return (type == ItemTypes_t::Bed);
}

bool ItemType::isRune() const noexcept
{
    return (type == ItemTypes_t::Rune);
}

bool ItemType::isPickupable() const noexcept
{
    return (allowPickupable || pickupable);
}

bool ItemType::isUseable() const noexcept
{
    return appearance->hasFlag(AppearanceFlag::Usable);
}

bool ItemType::hasSubType() const noexcept
{
    return (isFluidContainer() || isSplash() || stackable);
}

bool ItemType::usesSubType() const noexcept
{
    return isStackable() || isSplash() || isFluidContainer();
}

bool ItemType::isStackable() const noexcept
{
    return stackable;
}

bool ItemType::isBottom() const noexcept
{
    return hasFlag(AppearanceFlag::Bottom);
}

void ItemType::setFlag(ItemTypeFlag flag) noexcept
{
    flags |= flag;
}

void ItemType::addBrush(Brush *brush)
{
    DEBUG_ASSERT(brush != nullptr, "Brush should not be nullptr.");
    switch (brush->type())
    {
        case BrushType::Border:
            setFlag(ItemTypeFlag::InBorderBrush);
            borderBrush = static_cast<BorderBrush *>(brush);
            break;
        case BrushType::Mountain:
            setFlag(ItemTypeFlag::InMountainBrush);
            otherBrush = brush;
            break;
        case BrushType::Ground:
            setFlag(ItemTypeFlag::InGroundBrush);
            groundBrush = static_cast<GroundBrush *>(brush);
            break;
        case BrushType::Wall:
            setFlag(ItemTypeFlag::InWallBrush);
            otherBrush = brush;
            break;
        default:
            otherBrush = brush;
            break;
    }
}

Brush *ItemType::getBrush(BrushType type) const noexcept
{
    if (type == BrushType::Ground)
    {
        return groundBrush;
    }
    else if (type == BrushType::Border)
    {
        return borderBrush;
    }
    else if (otherBrush && otherBrush->type() == type)
    {
        return otherBrush;
    }
    else
    {
        return nullptr;
    }
}

Brush *ItemType::brush() const noexcept
{
    if (groundBrush)
    {
        return groundBrush;
    }
    else if (borderBrush)
    {
        return borderBrush;
    }
    else
    {
        return otherBrush;
    }
}