#include "vulkan_window.h"

#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

#include <QAction>
#include <QDialog>
#include <QDrag>
#include <QFocusEvent>
#include <QKeyCombination>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QQuickView>
#include <QWidget>

#include "../../vendor/rollbear-visit/visit.hpp"
#include "../brushes/border_brush.h"
#include "../brushes/brush.h"
#include "../brushes/creature_brush.h"
#include "../brushes/doodad_brush.h"
#include "../brushes/ground_brush.h"
#include "../brushes/mountain_brush.h"
#include "../brushes/wall_brush.h"
#include "../item_location.h"
#include "../logger.h"
#include "../map_renderer.h"
#include "../map_view.h"
#include "../position.h"
#include "../qt/logging.h"
#include "../settings.h"
#include "gui.h"
#include "mainwindow.h"
#include "qt_util.h"

// Static
std::unordered_set<const VulkanWindow *> VulkanWindow::instances;

VulkanWindow::VulkanWindow(std::shared_ptr<Map> map, EditorAction &editorAction)
    : QVulkanWindow(nullptr),
      vulkanInfo(this),
      editorAction(editorAction),
      mapView(std::make_unique<MapView>(std::make_unique<QtUtil::QtUiUtils>(this), editorAction, map)),
      scrollAngleBuffer(0)
{
    instances.emplace(this);

    connect(this, &VulkanWindow::scrollEvent, [=](int scrollDelta) { this->mapView->zoom(scrollDelta); });
    mapView->onMapItemDragStart<&VulkanWindow::mapItemDragStartEvent>(this);

    contextMenu = new QtContextMenu(this, widget);
    connect(contextMenu, &QtContextMenu::reopenRequest, this, &VulkanWindow::reopenContextMenuRequest);
    connect(contextMenu, &QtContextMenu::leftMouseReleaseOutside, this, &VulkanWindow::onLeftMouseReleaseOutside);

    setShortcut(Qt::Key_Escape, ShortcutAction::Escape);
    setShortcut(Qt::Key_Delete, ShortcutAction::Delete);
    setShortcut(Qt::ControlModifier, Qt::Key_0, ShortcutAction::ResetZoom);
    setShortcut(Qt::KeypadModifier, Qt::Key_Plus, ShortcutAction::FloorUp);
    setShortcut(Qt::KeypadModifier, Qt::Key_Minus, ShortcutAction::FloorDown);
    setShortcut(Qt::Key_Q, ShortcutAction::LowerFloorShade);
}

VulkanWindow::~VulkanWindow()
{
    instances.erase(this);
}

void VulkanWindow::shortcutPressedEvent(ShortcutAction action, QKeyEvent *event)
{
    switch (action)
    {
        case ShortcutAction::Escape:
            mapView->escapeEvent();
            break;
        case ShortcutAction::Delete:
            mapView->deleteSelectedItems();
            break;
        case ShortcutAction::ResetZoom:
            mapView->resetZoom();
            break;
        case ShortcutAction::FloorUp:
            mapView->floorUp();
            break;
        case ShortcutAction::FloorDown:
            mapView->floorDown();

            break;
        case ShortcutAction::LowerFloorShade:
            mapView->toggleViewOption(MapView::ViewOption::ShadeLowerFloors);
            break;
    }
}

//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>Events>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>

void VulkanWindow::mapItemDragStartEvent(Tile *tile, Item *item)
{
    ItemDrag::MapItem mapItem(mapView.get(), tile, item);
    dragOperation.emplace(ItemDrag::DragOperation::create(std::move(mapItem), mapView.get(), this));
    dragOperation->setRenderCondition([this] { return !this->containsMouse(); });
    dragOperation->start();
}

void VulkanWindow::shortcutReleasedEvent(ShortcutAction action, QKeyEvent *event)
{
    switch (action)
    {
        // case ShortcutAction::Pan:
        // {
        //     auto pan = mapView->editorAction.as<MouseAction::Pan>();
        //     if (pan)
        //     {
        //         unsetCursor();
        //         mapView->editorAction.setPrevious();
        //     }
        //     break;
        // }
        default:
            break;
    }
}

void VulkanWindow::mouseDoubleClickEvent(QMouseEvent *ev)
{
    if (mapView->singleThingSelected())
    {
        mainWindow->setPropertyPanelVisible(true);
    }
}

void VulkanWindow::mousePressEvent(QMouseEvent *event)
{
    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);
    // VME_LOG_D("VulkanWindow::mousePressEvent");
    mouseState.buttons = event->buttons();

    switch (event->button())
    {
        case Qt::MouseButton::LeftButton:
            if (!contextMenu->isHidden())
            {
                closeContextMenu();
            }
            else
            {
                mapView->mousePressEvent(QtUtil::vmeMouseEvent(event));
            }

            break;
        default:
            break;
    }

    event->ignore();
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *event)
{
    // VME_LOG_D("VulkanWindow Move: " << event->pos());
    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);

    mouseState.buttons = event->buttons();
    mapView->mouseMoveEvent(QtUtil::vmeMouseEvent(event));

    // Update drag
    if (dragOperation)
    {
        dragOperation->mouseMoveEvent(event);
    }

    auto pos = event->scenePosition();
    util::Point<float> mousePos(pos.x(), pos.y());
    emit mousePosChanged(mousePos);

    event->ignore();
    QVulkanWindow::mouseMoveEvent(event);
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::RightButton)
    {
        if (contextMenu->isHidden())
        {
            editorAction.reset();
            openContextMenu(event->globalPosition().toPoint());
        }
    }
    else if (event->button() == Qt::MouseButton::LeftButton)
    {
        // VME_LOG_D("VulkanWindow::mouseReleaseEvent");
        // Propagate drag operation
        if (dragOperation)
        {
            bool accepted = dragOperation->sendDropEvent(event);
            if (!accepted)
            {
                mapView->editorAction.as<MouseAction::DragDropItem>()->moveDelta.emplace(PositionConstants::Zero);
            }

            mapView->editorAction.unlock();
            mapView->editorAction.setPrevious();

            dragOperation.reset();
        }

        mouseState.buttons = event->buttons();
        mapView->mouseReleaseEvent(QtUtil::vmeMouseEvent(event));
    }
}

void VulkanWindow::wheelEvent(QWheelEvent *event)
{
    /*
    The minimum rotation amount for a scroll to be registered, in eighths of a degree.
    For example, 120 MinRotationAmount = (120 / 8) = 15 degrees of rotation.
  */
    const int MinRotationAmount = 120;

    // The relative amount that the wheel was rotated, in eighths of a degree.
    const int deltaY = event->angleDelta().y();

    scrollAngleBuffer += deltaY;
    if (std::abs(scrollAngleBuffer) >= MinRotationAmount)
    {
        emit scrollEvent(scrollAngleBuffer / 8);
        scrollAngleBuffer = 0;
    }
}

void VulkanWindow::keyReleaseEvent(QKeyEvent *e)
{
    if (e->isAutoRepeat())
        return;

    auto shortcut = shortcuts.find(e->key() | e->modifiers());
    if (shortcut != shortcuts.end())
    {
        shortcutReleasedEvent(shortcut->second);
        return;
    }

    switch (e->key())
    {
        case Qt::Key_Control:
        {
            auto brush = mapView->editorAction.as<MouseAction::MapBrush>();
            if (brush)
            {
                brush->erase = false;
            }
        }
    }
}

void VulkanWindow::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            e->ignore();
            emit keyPressedEvent(e);
            break;
        case Qt::Key_Control:
        {
            auto brush = mapView->editorAction.as<MouseAction::MapBrush>();
            if (brush)
            {
                brush->erase = true;
            }
            break;
        }
        default:
            e->ignore();
            QVulkanWindow::keyPressEvent(e);
            break;
    }
}

void VulkanWindow::setShortcut(Qt::KeyboardModifiers modifiers, Qt::Key key, ShortcutAction shortcut)
{
    int combination = QKeyCombination(modifiers, key).toCombined();
    shortcuts.emplace(combination, shortcut);
    shortcutActionToKeyCombination.emplace(shortcut, combination);
}

void VulkanWindow::setShortcut(Qt::Key key, ShortcutAction shortcut)
{
    int combination = QKeyCombination(key).toCombined();
    shortcuts.emplace(combination, shortcut);
    shortcutActionToKeyCombination.emplace(shortcut, combination);
}

std::optional<ShortcutAction> VulkanWindow::getShortcutAction(QKeyEvent *event) const
{
    auto shortcut = shortcuts.find(event->key() | event->modifiers());

    return shortcut != shortcuts.end() ? std::optional<ShortcutAction>(shortcut->second) : std::nullopt;
}

void VulkanWindow::dragEnterEvent(QDragEnterEvent *event)
{
    auto eventMimeData = event->mimeData();
    if (eventMimeData->hasFormat(ItemDrag::DraggableItemFormat))
    {
        DEBUG_ASSERT(util::hasDynamicType<ItemDrag::MimeData *>(eventMimeData), "Incorrect MimeData type.");

        auto mimeData = static_cast<const ItemDrag::MimeData *>(eventMimeData);
        mapView->overlay().draggedItem = mimeData->draggableItem->item();
    }
}

void VulkanWindow::dragMoveEvent(QDragMoveEvent *event)
{
    // TODO Only draw when game position changes
    mapView->requestDraw();
}

void VulkanWindow::dragLeaveEvent(QDragLeaveEvent *event)
{
    mapView->overlay().draggedItem = nullptr;
}

void VulkanWindow::dropEvent(QDropEvent *event)
{
    auto eventMimeData = event->mimeData();
    if (!eventMimeData->hasFormat(ItemDrag::DraggableItemFormat))
    {
        event->ignore();
        return;
    }

    DEBUG_ASSERT(util::hasDynamicType<ItemDrag::MimeData *>(eventMimeData), "Incorrect MimeData type.");
    auto mimeData = static_cast<const ItemDrag::MimeData *>(eventMimeData);

    mapView->overlay().draggedItem = nullptr;

    auto droppedItem = mimeData->draggableItem.get();

    if (util::hasDynamicType<ItemDrag::MapItem *>(droppedItem))
    {
        VME_LOG_D("dropEvent: MapItem");
        event->accept();
    }
    else if (util::hasDynamicType<ItemDrag::ContainerItemDrag *>(droppedItem))
    {
        VME_LOG_D("dropEvent: ContainerItemDrag");
        event->accept();
        auto containerDrag = static_cast<ItemDrag::ContainerItemDrag *>(droppedItem);

        mapView->history.beginTransaction(TransactionType::MoveItems);

        ContainerLocation move(containerDrag->position, containerDrag->tileIndex, std::move(containerDrag->containerIndices));

        auto &tile = mapView->getOrCreateTile(mapView->mouseGamePos());

        mapView->moveFromContainerToMap(move, tile);

        mapView->history.endTransaction(TransactionType::MoveItems);
    }
    else
    {
        event->ignore();
        return;
    }
}

bool VulkanWindow::event(QEvent *event)
{
    // if (!(event->type() == QEvent::UpdateRequest) && !(event->type() == QEvent::MouseMove))
    // {
    //     qDebug() << "[" << QString(debugName.c_str()) << "] " << event->type() << " { " << mapToGlobal(position()) << " }";
    // }

    switch (event->type())
    {
        case QEvent::Enter:
        {
            bool panning = mapView->editorAction.is<MouseAction::Pan>();
            if (panning)
            {
                setCursor(Qt::OpenHandCursor);
            }
            mapView->setUnderMouse(true);
            break;
        }

        case QEvent::DragEnter:
            dragEnterEvent(static_cast<QDragEnterEvent *>(event));
            mapView->setUnderMouse(true);
            mapView->dragEnterEvent();
            break;

        case QEvent::DragMove:
            dragMoveEvent(static_cast<QDragMoveEvent *>(event));
            break;

        case QEvent::Leave:
        {

            mapView->setUnderMouse(false);
            break;
        }

        case QEvent::DragLeave:
            dragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
            mapView->setUnderMouse(false);
            mapView->dragLeaveEvent();
            break;

        case QEvent::Drop:
            dropEvent(static_cast<QDropEvent *>(event));
            break;

        case QEvent::ShortcutOverride:
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            auto action = getShortcutAction(keyEvent);
            if (action)
            {
                shortcutPressedEvent(action.value());
                return true;
            }
            break;
        }

        default:
            event->ignore();
            break;
    }

    return QVulkanWindow::event(event);
}

void VulkanWindow::eyedrop(const Position position) const
{
    TileThing topThing = mapView->map()->getTopThing(position);

    rollbear::visit(
        util::overloaded{
            [this](Item *item) {
                bool success = mainWindow->selectBrush(Brush::getOrCreateRawBrush(item->serverId()));

                // If the brush was not found in any tileset in any palette
                if (!success)
                {
                    mapView->editorAction.setRawBrush(item->serverId());
                }

                mapView->requestDraw();
            },
            [this](Creature *creature) {
                mainWindow->selectBrush(Brush::getCreatureBrush(creature->creatureType.id()));
                mapView->requestDraw();
            },

            [](const auto &arg) {
            }},
        topThing);
}

void VulkanWindow::lostFocus()
{
    if (contextMenu)
    {
        closeContextMenu();
    }
}

QWidget *VulkanWindow::wrapInWidget(QWidget *parent)
{
    QWidget *wrapper = QWidget::createWindowContainer(this, parent);
    QtUtil::setMapView(*wrapper, mapView.get());
    QtUtil::setVulkanWindow(*wrapper, this);
    wrapper->setObjectName("VulkanWindow wrapper");

    widget = wrapper;

    return wrapper;
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    if (!renderer)
    {
        // Memory deleted by QT when QT closes
        renderer = new VulkanWindow::Renderer(*this);
    }

    return renderer;
}

QRect VulkanWindow::localGeometry() const
{
    return QRect(QPoint(0, 0), QPoint(width(), height()));
}

void VulkanWindow::closeContextMenu()
{
    contextMenu->close();
}

void VulkanWindow::reopenContextMenuRequest(QPoint globalPos)
{
    if (!localGeometry().contains(mapFromGlobal(globalPos)))
    {
        contextMenu->close();
        return;
    }
    else
    {
        openContextMenu(globalPos);
    }
}

void VulkanWindow::onLeftMouseReleaseOutside(QMouseEvent *event)
{
    QPoint mapScreenPos = mapFromGlobal(event->globalPos());

    if (localGeometry().contains(mapScreenPos))
    {
        Position pos = mapView->toPosition(util::Point(mapScreenPos.x(), mapScreenPos.y()));
        mapView->editorAction.reset();

        VME::MouseButtons buttons = enum_conversion::vmeButtons(event->buttons());
        VME::ModifierKeys modifiers = enum_conversion::vmeModifierKeys(event->modifiers());

        ScreenPosition mouseEventPos(mapScreenPos.x(), mapScreenPos.y());
        mapView->mousePressEvent(VME::MouseEvent(mouseEventPos, buttons, modifiers));
    }
}

void VulkanWindow::openContextMenu(QPoint position)
{
    auto menu = contextMenu;
    menu->close();
    menu->clear();

    Position gamePos = mapView->mouseGamePos();

    auto addAction = [menu](QString text, QKeySequence shortcut = 0, std::function<void()> f = nullptr) -> QAction * {
        QAction *action = new QAction(text, menu);
        if (f)
        {
            connect(action, &QAction::triggered, f);
        }
        if (shortcut != 0)
        {
            action->setShortcut(shortcut);
        }

        menu->addAction(action);
        return action;
    };

    auto addDisabledAction = [menu](QString text, QKeySequence shortcut = 0) -> QAction * {
        QAction *action = new QAction(text, menu);
        if (shortcut != 0)
        {
            action->setShortcut(shortcut);
        }
        action->setDisabled(true);

        menu->addAction(action);
        return action;
    };

    Tile *tile = mapView->getTile(gamePos);
    if (tile)
    {
        mapView->selectTopThing(gamePos);

        // Cut Copy Copy Position Paste Delete, Browse Field
        addAction("Cut", Qt::CTRL | Qt::Key_X, [this]() {
            mainWindow->copySelection();
            mapView->deleteSelectedItems();
        });
        addAction("Copy", Qt::CTRL | Qt::Key_C, [this]() { mainWindow->copySelection(); });
        addAction("Copy Position");
        addAction("Paste", Qt::CTRL | Qt::Key_V, [this]() {
            if (mainWindow->hasCopyBuffer() && mapView)
            {
                this->editorAction.set(MouseAction::PasteMapBuffer(&mainWindow->getMapCopyBuffer()));
                mapView->requestDraw();
            }
        });

        addAction("Delete", Qt::Key_Delete, [this]() { mapView->deleteSelectedItems(); });

        auto addBrushOption = [this, &addAction](QString text, Brush *brush) {
            addAction(text, 0, [this, brush]() {
                mainWindow->selectBrush(brush);
                // this->editorAction.set(MouseAction::MapBrush(brush));
                mapView->commitTransaction(TransactionType::Selection, [this] { mapView->clearSelection(); });
            });
        };

        // Raw brush
        auto topItem = tile->getTopItem();
        if (topItem)
        {

            uint32_t id = topItem->serverId();
            Brush *brush = Brush::getOrCreateRawBrush(id);
            addBrushOption("Select RAW", brush);
        }

        // Wall brush
        WallBrush *wallBrush = Brush::getWallBrush(*tile);
        if (wallBrush)
        {
            addBrushOption("Select Ground Brush", wallBrush);
        }

        // Border brush
        BorderBrush *borderBrush = Brush::getBorderBrush(*tile);
        if (borderBrush)
        {
            addBrushOption("Select Border Brush", borderBrush);
        }

        // Doodad brush
        DoodadBrush *doodadBrush = Brush::getDoodadBrush(*tile);
        if (doodadBrush)
        {
            addBrushOption("Select Doodad Brush", doodadBrush);
        }

        // Ground brush
        GroundBrush *groundBrush = Brush::getGroundBrush(*tile);
        if (groundBrush)
        {
            addBrushOption("Select Ground Brush", groundBrush);
        }

        // Ground brush
        MountainBrush *mountainBrush = Brush::getMountainBrush(*tile);
        if (mountainBrush)
        {
            addBrushOption("Select Mountain Brush", mountainBrush);
        }
    }
    else
    {
        mapView->commitTransaction(TransactionType::Selection, [this] { mapView->clearSelection(); });
        addDisabledAction("Cut", Qt::CTRL | Qt::Key_X);
        addDisabledAction("Copy", Qt::CTRL | Qt::Key_C);
        addAction("Copy Position");
        if (mainWindow->hasCopyBuffer())
        {
            VME_LOG_D("has copy buffer");
            addAction("Paste", Qt::CTRL | Qt::Key_V, [this]() {
                if (mainWindow->hasCopyBuffer() && mapView)
                {
                    this->editorAction.set(MouseAction::PasteMapBuffer(&mainWindow->getMapCopyBuffer()));
                    mapView->requestDraw();
                }
            });
        }
        else
        {
            addDisabledAction("Paste", Qt::CTRL | Qt::Key_V);
        }
        addDisabledAction("Delete", Qt::Key_Delete);
    }

    menu->popup(position);
}

MapView *VulkanWindow::getMapView() const
{
    return mapView.get();
}

util::Size VulkanWindow::vulkanSwapChainImageSize() const
{
    QSize size = swapChainImageSize();
    return util::Size(size.width(), size.height());
}

void VulkanWindow::updateVulkanInfo()
{
    vulkanInfo.update();
}

bool VulkanWindow::containsMouse() const
{
    QSize windowSize = size();
    auto mousePos = mapFromGlobal(QCursor::pos());

    return (0 <= mousePos.x() && mousePos.x() <= windowSize.width()) && (0 <= mousePos.y() && mousePos.y() <= windowSize.height());
}

//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>ContextMenu>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>

QtContextMenu::QtContextMenu(VulkanWindow *window, QWidget *widget)
    : QMenu(widget)
{
}

bool QtContextMenu::selfClicked(QPoint pos) const
{
    return localGeometry().contains(pos);
}

void QtContextMenu::mouseReleaseEvent(QMouseEvent *event)
{
    switch (event->button())
    {
        case Qt::MouseButton::RightButton:
            if (!isHidden())
            {
                hide();
            }
            break;
    }

    QMenu::mouseReleaseEvent(event);
}

void QtContextMenu::mousePressEvent(QMouseEvent *event)
{
    switch (event->button())
    {
        case Qt::MouseButton::RightButton:
        {
            if (!selfClicked(event->pos()))
            {
                emit reopenRequest(event->globalPos());
            }
            break;
        }
        case Qt::MouseButton::LeftButton:
        {
            if (!selfClicked(event->pos()))
            {
                emit leftMouseReleaseOutside(event);
            }
            break;
        }
    }

    QMenu::mousePressEvent(event);
    // // Propagate the click event to the map window if appropriate
    // if (!selfClicked(event->pos()))
    // {
    //   auto posInWindow = window->mapFromGlobal(event->globalPos());
    //   VME_LOG_D("posInWindow: " << posInWindow);
    //   VME_LOG_D("Window geometry: " << window->geometry());
    //   if (window->localGeometry().contains(posInWindow.x(), posInWindow.y()))
    //   {
    //     VME_LOG_D("In window");
    //     window->mousePressEvent(event);
    //   }
    //   else
    //   {
    //     event->ignore();
    //     window->lostFocus();
    //   }
    // }
}

QRect QtContextMenu::localGeometry() const
{
    return QRect(QPoint(0, 0), QPoint(width(), height()));
}
QRect QtContextMenu::relativeGeometry() const
{
    VME_LOG_D("relativeGeometry");
    //  VME_LOG_D(parentWidget()->pos());
    QPoint p(geometry().left(), geometry().top());

    VME_LOG_D(parentWidget()->mapToGlobal(parentWidget()->pos()));

    VME_LOG_D("Top left: " << p);
    VME_LOG_D(mapToParent(p));

    return geometry();
}

//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>Renderer>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>

VulkanWindow::Renderer::Renderer(VulkanWindow &window)
    : window(window),
      renderer(window.vulkanInfo, window.mapView.get()) {}

void VulkanWindow::Renderer::initResources()
{
    renderer.initResources(window.colorFormat());
};

void VulkanWindow::Renderer::initSwapChainResources()
{
    renderer.initSwapChainResources(window.vulkanSwapChainImageSize());
};

void VulkanWindow::Renderer::releaseSwapChainResources()
{
    renderer.releaseSwapChainResources();
};

void VulkanWindow::Renderer::releaseResources()
{
    renderer.releaseResources();
};

void VulkanWindow::Renderer::startNextFrame()
{
    renderer.setCurrentFrame(window.currentFrame());
    auto frame = renderer.currentFrame();
    frame->currentFrameIndex = window.currentFrame();
    frame->commandBuffer = window.currentCommandBuffer();
    frame->frameBuffer = window.currentFramebuffer();
    frame->mouseAction = window.mapView->editorAction.action();

    renderer.startNextFrame();
    if (renderer._containsAnimation && Settings::RENDER_ANIMATIONS)
    {
        QTimer::singleShot(50, [this]() { window.requestUpdate(); });
    }
}
