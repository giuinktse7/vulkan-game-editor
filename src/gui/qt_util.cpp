#include "qt_util.h"

#include <algorithm>
#include <memory>
#include <optional>

#include <QApplication>
#include <QLabel>
#include <QPainter>
#include <QSize>
#include <QWheelEvent>

#include "vulkan_window.h"

#include "../definitions.h"
#include "../items.h"
#include "../logger.h"
#include "../map_view.h"
#include "../qt/logging.h"
#include "main_application.h"

std::unordered_map<uint32_t, QPixmap> GuiImageCache::serverIdToPixmap;
std::unordered_map<uint32_t, std::unique_ptr<QImage>> GuiImageCache::textureIdToQImage;

namespace
{
    constexpr int InitialImageCacheCapacity = 4096;
}

void GuiImageCache::initialize()
{
    serverIdToPixmap.reserve(InitialImageCacheCapacity);
}

void GuiImageCache::cachePixmapForServerId(uint32_t serverId)
{
    get(serverId);
}

const QPixmap &GuiImageCache::get(uint32_t serverId)
{
    auto found = serverIdToPixmap.find(serverId);
    if (found == serverIdToPixmap.end())
    {
        serverIdToPixmap.try_emplace(serverId, QtUtil::itemPixmap(serverId));
    }

    return serverIdToPixmap.at(serverId);
}

QImage *GuiImageCache::getOrCreateQImageForTexture(const Texture &texture)
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

namespace
{
    std::unique_ptr<QPixmap> blackSquare;
    std::optional<ItemImageData> blackSquareImage;
    std::unique_ptr<QImage> black32x32;

    ItemImageData &getBlackSquareImage()
    {
        if (!blackSquareImage)
        {
            blackSquareImage = {};

            blackSquareImage->rect = QRect(0, 0, 32, 32);

            black32x32 = std::make_unique<QImage>(32, 32, QImage::Format::Format_ARGB32);
            black32x32->fill(QColor(0, 0, 0, 255));
            blackSquareImage->image = black32x32.get();
        }

        return *blackSquareImage;
    }

    QPixmap blackSquarePixmap()
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

} // namespace

QMetaObject::Connection QtUtil::QmlBind::connect(QObject *sender, const char *signal, F &&f)
{
    if (!sender)
        return {};

    return QObject::connect(sender, signal, new QmlBind(std::move(f), sender), SLOT(call()));
}

QtUtil::QmlBind::QmlBind(F &&f, QObject *parent)
    : QObject(parent), f(std::move(f)) {}

ScreenPosition QtUtil::QtUiUtils::mouseScreenPosInView()
{
    auto pos = window->mapFromGlobal(QCursor::pos());
    return ScreenPosition(pos.x(), pos.y());
}

VME::ModifierKeys QtUtil::QtUiUtils::modifiers() const
{
    return enum_conversion::vmeModifierKeys(QApplication::keyboardModifiers());
}

void QtUtil::QtUiUtils::waitForDraw(std::function<void()> f)
{
    window->waitingForDraw.emplace(f);
}

ItemImageData QtUtil::itemImageData(uint32_t serverId, uint8_t subtype)
{
    if (!Items::items.validItemType(serverId))
    {
        return getBlackSquareImage();
    }

    ItemType *t = Items::items.getItemTypeByServerId(serverId);
    auto info = subtype == 0 ? t->getTextureInfo(TextureInfo::CoordinateType::Unnormalized)
                             : t->getTextureInfoForSubtype(subtype, TextureInfo::CoordinateType::Unnormalized);

    return itemImageData(info);
}

QString QtUtil::resourcePath(Brush *brush)
{
    auto resource = brush->brushResource();
    switch (resource.type)
    {
        case BrushResourceType::ItemType:
            if (resource.variant != 0)
            {
                auto s = std::format("image://itemTypes/{}:{}", resource.id, static_cast<int>(resource.variant));
                return QString::fromStdString(s);
            }
            else
            {
                auto s = std::format("image://itemTypes/{}", resource.id);
                return QString::fromStdString(s);
            }
        case BrushResourceType::Creature:
            auto s = std::format("image://creatureLooktypes/{}:{}", resource.id, static_cast<int>(resource.variant));
            return QString::fromStdString(s);
    }

    ABORT_PROGRAM("Could not determine resource type.");
}

ItemImageData QtUtil::itemImageData(Brush *brush)
{
    auto resource = brush->brushResource();
    switch (resource.type)
    {
        case BrushResourceType::ItemType:
            return itemImageData(resource.id, resource.variant);
        case BrushResourceType::Creature:
            DEBUG_ASSERT(resource.variant >= 0 && resource.variant <= 3, std::format("Invalid creature direction: {}", resource.variant));
            const CreatureType *creatureType = Creatures::creatureType(resource.id);

            auto info = Creatures::creatureType(resource.id)->getTextureInfo(0, static_cast<Direction>(resource.variant), TextureInfo::CoordinateType::Unnormalized);

            if (creatureType->hasColorVariation())
            {
                return itemImageData(info.window, info.atlas->getTexture(creatureType->outfitId()));
            }
            else
            {
                return itemImageData(info);
            }
    }
}

ItemImageData QtUtil::itemImageData(const TextureWindow &window, const Texture &texture)
{
    QImage *image = GuiImageCache::getOrCreateQImageForTexture(texture);
    return {QRect(window.x0, window.y0, window.x1, window.y1), image};
}

ItemImageData QtUtil::itemImageData(const TextureInfo &info)
{
    return itemImageData(info.window, info.atlas->getOrCreateTexture());
}

QPixmap QtUtil::itemPixmap(uint32_t serverId, uint8_t subtype)
{
    if (!Items::items.validItemType(serverId))
    {
        return blackSquarePixmap();
    }

    ItemType *t = Items::items.getItemTypeByServerId(serverId);
    auto info = t->usesSubType() ? t->getTextureInfoForSubtype(subtype, TextureInfo::CoordinateType::Unnormalized)
                                 : t->getTextureInfo(TextureInfo::CoordinateType::Unnormalized);

    return thingPixmap(info);
}

QPixmap QtUtil::itemPixmap(const Position &pos, const Item &item)
{
    auto info = item.getTextureInfo(pos, TextureInfo::CoordinateType::Unnormalized);
    return thingPixmap(info);
}

QPixmap QtUtil::thingPixmap(const TextureWindow &textureWindow, const Texture &texture, uint16_t spriteWidth, uint16_t spriteHeight)
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

QPixmap QtUtil::thingPixmap(const TextureInfo &info)
{
    return thingPixmap(info.window, info.atlas->getOrCreateTexture(), info.atlas->spriteWidth, info.atlas->spriteHeight);
}

QPixmap QtUtil::creaturePixmap(uint32_t looktype, Direction direction)
{
    auto creatureType = Creatures::creatureType(looktype);
    if (!creatureType)
    {
        return blackSquarePixmap();
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

MainApplication *QtUtil::qtApp()
{
    return (MainApplication *)(QApplication::instance());
}

std::optional<int> QtUtil::ScrollState::scroll(QWheelEvent *event)
{
    // The relative amount that the wheel was rotated, in eighths of a degree.
    const int deltaY = event->angleDelta().y();
    amountBuffer += deltaY;

    if (std::abs(amountBuffer) < minRotationDelta)
        return {};

    int result = amountBuffer / QtMinimumWheelDelta;
    amountBuffer = amountBuffer % QtMinimumWheelDelta;

    return result;
}

QPixmap CreatureImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    auto parts = id.split(':');

    uint32_t looktype;
    uint8_t direction = to_underlying(Direction::South);

    bool success = false;

    // No direction if only one part
    if (parts.size() == 1)
    {
        looktype = parts.at(0).toInt(&success);
    }
    else
    {
        DEBUG_ASSERT(parts.size() == 2, "Must have 2 parts here; a looktype and a direction id");
        bool ok;
        looktype = parts.at(0).toInt(&ok);
        if (ok)
        {
            int parsedSubtype = parts.at(1).toInt(&success);
            if (success)
            {
                DEBUG_ASSERT(0 <= direction && direction <= 3, "direction id out of bounds.");
                direction = static_cast<uint8_t>(parsedSubtype);
            }
        }
    }

    if (!success)
    {
        QPixmap pixmap(32, 32);
        pixmap.fill(QColor("black").rgba());
        return pixmap;
    }

    return QtUtil::creaturePixmap(looktype, static_cast<Direction>(direction));
}

QPixmap ItemTypeImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    //id is either 'serverId' or 'serverId:subtype'.
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

    if (!success)
    {
        QPixmap pixmap(32, 32);
        pixmap.fill(QColor("black").rgba());
        return pixmap;
    }

    return QtUtil::itemPixmap(serverId, subtype);
}

MapView *QtUtil::associatedMapView(QWidget *widget)
{
    if (!widget)
        return nullptr;
    QVariant prop = widget->property(QtUtil::PropertyName::MapView);
    if (prop.isNull())
        return nullptr;

    MapView *mapView = static_cast<MapView *>(prop.value<void *>());
#ifdef _DEBUG_VME
    bool isInstance = MapView::isInstance(mapView);

    std::ostringstream msg;
    msg << "The property QtUtil::PropertyName::MapView for widget " << widget << " must contain a MapView pointer.";
    DEBUG_ASSERT(isInstance, msg.str());

    return mapView;
#else
    return MapView::isInstance(mapView) ? mapView : nullptr;
#endif
}

VulkanWindow *QtUtil::associatedVulkanWindow(QWidget *widget)
{
    if (!widget)
        return nullptr;
    QVariant prop = widget->property(QtUtil::PropertyName::VulkanWindow);
    if (prop.isNull())
        return nullptr;

    VulkanWindow *vulkanWindow = static_cast<VulkanWindow *>(prop.value<void *>());
#ifdef _DEBUG_VME
    bool isInstance = VulkanWindow::isInstance(vulkanWindow);

    std::ostringstream msg;
    msg << "The property QtUtil::PropertyName::MapView for widget " << widget << " must contain a MapView pointer.";
    DEBUG_ASSERT(isInstance, msg.str());

    return vulkanWindow;
#else
    return VulkanWindow::isInstance(vulkanWindow) ? vulkanWindow : nullptr;
#endif
    return vulkanWindow;
}
