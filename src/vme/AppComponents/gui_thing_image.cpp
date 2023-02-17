#include "gui_thing_image.h"

#include <QImage>
#include <QPainter>
#include <QPixmap>
#include <QRect>

#include "core/creature.h"
#include "core/graphics/texture_atlas.h"
#include "core/item.h"
#include "core/items.h"

vme_unordered_map<uint32_t, QPixmap> GUIImageCache::serverIdToPixmap;
vme_unordered_map<uint32_t, std::unique_ptr<QImage>> GUIImageCache::textureIdToQImage;
std::unique_ptr<QPixmap> GUIImageCache::blackSquare;

QtTextureArea::QtTextureArea(const TextureWindow &window, const Texture &texture)
    : rect(window.x0, window.y0, window.x1, window.y1), image(GUIImageCache::getOrCreateQImageForTexture(texture)) {}

QtTextureArea::QtTextureArea(QRect rect, QImage *image)
    : rect(rect), image(image) {}

namespace
{
    constexpr int InitialImageCacheCapacity = 4096;
} // namespace

void GUIImageCache::initialize()
{
    serverIdToPixmap.reserve(InitialImageCacheCapacity);
}

void GUIImageCache::cachePixmapForServerId(uint32_t serverId)
{
    get(serverId);
}

const QPixmap &GUIImageCache::get(uint32_t serverId)
{
    auto found = serverIdToPixmap.find(serverId);
    if (found == serverIdToPixmap.end())
    {
        serverIdToPixmap.try_emplace(serverId, GUIThingImage::itemPixmap(serverId));
    }

    return serverIdToPixmap.at(serverId);
}

QImage *GUIImageCache::getOrCreateQImageForTexture(const Texture &texture)
{
    auto found = textureIdToQImage.find(texture.id());
    if (found == textureIdToQImage.end())
    {
        const uint8_t *pixelData = texture.pixels().data();
        auto image = std::make_unique<QImage>(pixelData, 12 * 32, 12 * 32, 384 * 4, QImage::Format::Format_ARGB32);
        textureIdToQImage.try_emplace(texture.id(), std::move(image));
    }

    return textureIdToQImage.at(texture.id()).get();
}

QImage GUIThingImage::getCreatureTypeImage(const CreatureType &creatureType, Direction direction)
{
    QImage image(QSize(32, 32), QImage::Format::Format_ARGB32);
    image.fill(QColor("transparent"));
    QPainter painter(&image);

    auto getLayer = [&painter](const CreatureType *creatureType, int posture, int addonType, Direction direction) {
        auto info = creatureType->getTextureInfo(0, posture, addonType, direction, TextureInfo::CoordinateType::Unnormalized);

        const Texture &texture = creatureType->hasColorVariation()
                                     ? info.getTexture(creatureType->outfitId())
                                     : info.getTexture();

        auto rect = QRect(info.window.x0, info.window.y0, info.window.x1, info.window.y1);
        QImage img = GUIImageCache::getOrCreateQImageForTexture(texture)->copy(rect);
        if (rect.width() > 32 || rect.height() > 32)
        {
            img = img.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        painter.drawImage(QPoint(0, 0), img);
    };

    uint8_t posture = creatureType.hasMount() ? 1 : 0;
    // uint8_t posture = 1;

    // Mount?
    if (creatureType.hasMount())
    {
        // Draw mount first
        getLayer(Creatures::creatureType(creatureType.mountLooktype()), 0, 0, direction);

        getLayer(&creatureType, 1, 0, direction);
    }
    else
    {
        // No mount, draw the base outfit
        getLayer(&creatureType, posture, 0, direction);
    }

    // Addons?
    if (creatureType.hasAddon(Outfit::Addon::First))
    {
        getLayer(&creatureType, posture, 1, direction);
    }
    if (creatureType.hasAddon(Outfit::Addon::Second))
    {
        getLayer(&creatureType, posture, 2, direction);
    }

    return image.mirrored();
}

QImage GUIThingImage::getItemTypeImage(const ItemType &itemType, uint8_t subtype)
{
    TextureInfo info = subtype > 1 ? itemType.getTextureInfoForSubtype(subtype, TextureInfo::CoordinateType::Unnormalized)
                                   : itemType.getTextureInfo(TextureInfo::CoordinateType::Unnormalized);

    QRect rect(info.window.x0, info.window.y0, info.window.x1, info.window.y1);
    QImage *textureImage = GUIImageCache::getOrCreateQImageForTexture(info.getTexture());

    QImage image = textureImage->copy(rect).mirrored();

    if (rect.width() > 32 || rect.height() > 32)
    {
        image = image.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return image;
}

QImage GUIThingImage::getItemTypeImage(uint32_t serverId, uint8_t subtype)
{
    auto itemType = Items::items.getItemTypeByServerId(serverId);
    DEBUG_ASSERT(itemType != nullptr, std::format("Invalid ItemType serverId: {}", serverId));

    return getItemTypeImage(*itemType, subtype);
}

QPixmap GUIThingImage::itemPixmap(uint32_t serverId, uint8_t subtype)
{
    if (!Items::items.validItemType(serverId))
    {
        return GUIImageCache::blackSquarePixmap();
    }

    ItemType *t = Items::items.getItemTypeByServerId(serverId);
    auto info = t->usesSubType() ? t->getTextureInfoForSubtype(subtype, TextureInfo::CoordinateType::Unnormalized)
                                 : t->getTextureInfo(TextureInfo::CoordinateType::Unnormalized);

    return thingPixmap(info);
}

QPixmap GUIThingImage::itemPixmap(const Position &pos, const Item &item)
{
    auto info = item.getTextureInfo(pos, TextureInfo::CoordinateType::Unnormalized);
    return thingPixmap(info);
}

QPixmap GUIThingImage::thingPixmap(const TextureWindow &textureWindow, const Texture &texture, uint16_t spriteWidth, uint16_t spriteHeight)
{
    const uint8_t *pixelData = texture.pixels().data();

    QRect textureRegion(textureWindow.x0, textureWindow.y0, textureWindow.x1, textureWindow.y1);

    QImage sprite = QImage(pixelData, 12 * 32, 12 * 32, 384 * 4, QImage::Format::Format_ARGB32)
                        .copy(textureRegion)
                        .mirrored();

    auto width = spriteWidth;
    auto height = spriteHeight;

    if (width == 32 && height == 32)
    {
        return QPixmap::fromImage(std::move(sprite));
    }
    else if (width == 64 && height == 64)
    {
        return QPixmap::fromImage(sprite.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else if (width == 64 && height == 32)
    {
        return QPixmap::fromImage(sprite.scaled(32, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else if (width == 32 && height == 64)
    {
        return QPixmap::fromImage(sprite.scaled(16, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else
    {
        VME_LOG_ERROR(std::format("Warning: Unknown sprite size: (width: {}, height: {})", width, height));
        return QPixmap::fromImage(std::move(sprite));
    }
}

QPixmap GUIThingImage::thingPixmap(const TextureInfo &info)
{
    return thingPixmap(info.window, info.atlas->getOrCreateTexture(), info.atlas->spriteWidth, info.atlas->spriteHeight);
}

QPixmap GUIImageCache::blackSquarePixmap()
{
    if (!blackSquare)
    {
        QImage image(32, 32, QImage::Format::Format_ARGB32);
        image.fill(QColor(0, 0, 0, 255));

        QPixmap pixmap = QPixmap::fromImage(image);

        blackSquare = std::make_unique<QPixmap>(std::move(pixmap));
    }

    return *blackSquare;
}

QPixmap GUIThingImage::creaturePixmap(const CreatureType *creatureType, Direction direction)
{
    if (!creatureType)
    {
        return GUIImageCache::blackSquarePixmap();
    }

    auto info = creatureType->getTextureInfo(0, direction, TextureInfo::CoordinateType::Unnormalized);

    if (creatureType->hasColorVariation())
    {
        TextureAtlas *atlas = info.atlas;
        return thingPixmap(info.window, atlas->getTexture(creatureType->outfitId()), atlas->spriteWidth, atlas->spriteHeight);
    }
    else
    {
        return thingPixmap(info);
    }
}

QPixmap GUIThingImage::creaturePixmap(uint32_t looktype, Direction direction)
{
    auto creatureType = Creatures::creatureType(looktype);
    if (!creatureType)
    {
        return GUIImageCache::blackSquarePixmap();
    }

    auto info = creatureType->getTextureInfo(0, direction, TextureInfo::CoordinateType::Unnormalized);

    if (creatureType->hasColorVariation())
    {
        TextureAtlas *atlas = info.atlas;
        return thingPixmap(info.window, atlas->getTexture(creatureType->outfitId()), atlas->spriteWidth, atlas->spriteHeight);
    }
    else
    {
        return thingPixmap(info);
    }
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>Image Providers for QML>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

QPixmap CreatureImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    /*
        Parts are: creatureTypeId[:direction]
    */
    auto parts = id.split(':');

    if (parts.size() != 1 && parts.size() != 2)
    {
        ABORT_PROGRAM(std::format("Malformed looktype resource id: {}. Must be [creatureTypeId] or [creatureTypeId:direction].", id.toStdString()));
    }

    uint8_t direction = to_underlying(Direction::South);

    bool success = false;
    bool ok = false;

    std::string creatureTypeId = parts.at(0).toStdString();
    CreatureType *creatureType = Creatures::creatureType(creatureTypeId);

    if (!creatureType)
    {
        VME_LOG_ERROR(std::format("There is no creatureType with id '{}'", creatureTypeId));
        return GUIImageCache::blackSquarePixmap();
    }

    if (parts.size() == 1)
    {
        success = true;
    }
    else if (parts.size() == 2)
    {
        int parsedDirection = parts.at(1).toInt(&success);
        if (success)
        {
            DEBUG_ASSERT(0 <= direction && direction <= 3, "direction id out of bounds.");
            direction = static_cast<uint8_t>(parsedDirection);
        }
    }

    if (!success)
    {
        return GUIImageCache::blackSquarePixmap();
    }

    return QPixmap::fromImage(GUIThingImage::getCreatureTypeImage(*creatureType, static_cast<Direction>(direction)));
}

QPixmap ItemTypeImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    // id is either 'serverId' or 'serverId:subtype'.
    auto parts = id.split(':');

    uint32_t serverId;
    uint8_t subtype = 1;

    bool success = false;

    // No subtype if only one part
    if (parts.size() == 1)
    {
        serverId = parts.at(0).toInt(&success);
    }
    else
    {
        DEBUG_ASSERT(parts.size() == 2, "Must have 2 parts here; a serverId and a subtype");
        bool ok;
        serverId = parts.at(0).toInt(&ok);
        if (ok)
        {
            int parsedSubtype = parts.at(1).toInt(&success);
            if (success)
            {
                DEBUG_ASSERT(0 <= subtype && subtype <= UINT8_MAX, "Subtype out of bounds.");
                subtype = static_cast<uint8_t>(parsedSubtype);
            }
        }
    }

    if (!(success && Items::items.validItemType(serverId)))
    {
        return GUIImageCache::blackSquarePixmap();
    }

    return QPixmap::fromImage(GUIThingImage::getItemTypeImage(serverId, subtype));
}