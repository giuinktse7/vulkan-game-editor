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
std::unordered_map<TextureAtlas *, std::unique_ptr<QImage>> GuiImageCache::atlasToQImage;

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

QImage *GuiImageCache::getOrCreateQImageForAtlas(TextureAtlas *atlas)
{
    auto found = atlasToQImage.find(atlas);
    if (found == atlasToQImage.end())
    {
        const uint8_t *pixelData = atlas->getOrCreateTexture().pixels().data();
        auto image = std::make_unique<QImage>(pixelData, 12 * 32, 12 * 32, 384 * 4, QImage::Format::Format_ARGB32);
        atlasToQImage.try_emplace(atlas, std::move(image));
    }

    return atlasToQImage.at(atlas).get();
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

ItemImageData QtUtil::itemImageData(const TextureInfo &info)
{
    TextureAtlas *atlas = info.atlas;
    QRect textureRegion(info.window.x0, info.window.y0, info.window.x1, info.window.y1);

    QImage *image = GuiImageCache::getOrCreateQImageForAtlas(info.atlas);

    if (atlas->spriteWidth == 32 && atlas->spriteHeight == 32)
    {
        return {textureRegion, image};
    }
    else
    {
        // return QPixmap::fromImage(sprite.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        return {textureRegion, image};
    }
}

QPixmap QtUtil::itemPixmap(uint32_t serverId, uint8_t subtype)
{
    if (!Items::items.validItemType(serverId))
    {
        return blackSquarePixmap();
    }

    ItemType *t = Items::items.getItemTypeByServerId(serverId);
    auto info = subtype == 0 ? t->getTextureInfo(TextureInfo::CoordinateType::Unnormalized)
                             : t->getTextureInfoForSubtype(subtype, TextureInfo::CoordinateType::Unnormalized);

    return itemPixmap(info);
}

QPixmap QtUtil::itemPixmap(const Position &pos, const Item &item)
{
    auto info = item.getTextureInfo(pos, TextureInfo::CoordinateType::Unnormalized);
    return itemPixmap(info);
}

QPixmap QtUtil::itemPixmap(const TextureInfo &info)
{
    TextureAtlas *atlas = info.atlas;

    const uint8_t *pixelData = atlas->getOrCreateTexture().pixels().data();

    QRect textureRegion(info.window.x0, info.window.y0, info.window.x1, info.window.y1);

    QImage sprite = QImage(pixelData, 12 * 32, 12 * 32, 384 * 4, QImage::Format::Format_ARGB32)
                        .copy(textureRegion)
                        .mirrored();

    if (atlas->spriteWidth == 32 && atlas->spriteHeight == 32)
    {
        return QPixmap::fromImage(std::move(sprite));
    }
    else
    {
        return QPixmap::fromImage(sprite.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
