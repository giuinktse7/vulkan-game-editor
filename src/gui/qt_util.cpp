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

#include "../brushes/creature_brush.h"
#include "../brushes/doodad_brush.h"
#include "../brushes/ground_brush.h"
#include "../brushes/raw_brush.h"

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

QString QtUtil::getItemTypeResourcePath(uint32_t serverId, uint8_t subtype)
{
    if (subtype == 0)
    {
        return QString::fromStdString(std::format("image://itemTypes/{}", serverId));
    }
    else
    {
        return QString::fromStdString(std::format("image://itemTypes/{}:{}", serverId, static_cast<int>(subtype)));
    }
}

QString QtUtil::getCreatureTypeResourcePath(const CreatureType &creatureType, Direction direction)
{
    return QString::fromStdString(std::format("image://creatureLooktypes/{}:{}", creatureType.id(), to_underlying(direction)));
}

QString QtUtil::resourcePath(Brush *brush)
{
    switch (brush->type())
    {
        case BrushType::Raw:
        {
            auto rawBrush = static_cast<RawBrush *>(brush);
            return getItemTypeResourcePath(rawBrush->serverId());
        }
        case BrushType::Ground:
        {
            auto groundBrush = static_cast<GroundBrush *>(brush);
            return getItemTypeResourcePath(groundBrush->iconServerId());
        }
        case BrushType::Doodad:
        {
            auto doodadBrush = static_cast<DoodadBrush *>(brush);
            return getItemTypeResourcePath(doodadBrush->iconServerId());
        }
        case BrushType::Creature:
        {
            auto creatureBrush = static_cast<CreatureBrush *>(brush);
            return getCreatureTypeResourcePath(*creatureBrush->creatureType);
        }
        default:
            break;
    }

    ABORT_PROGRAM("Could not determine resource type.");
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
