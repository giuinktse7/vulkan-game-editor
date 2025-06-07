#include "qml_map_item.h"

#include <QGuiApplication>
#include <QTimer>

#include "core/brushes/brush.h"
#include "core/brushes/brushes.h"
#include "core/brushes/creature_brush.h"
#include "core/const.h"
#include "core/logger.h"
#include "core/map_renderer.h"
#include "core/render/render_coordinator.h"
#include "core/settings.h"
#include "core/time_util.h"
#include "core/vendor/rollbear-visit/visit.hpp"
#include "qml_ui_utils.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <qnamespace.h>
#include <qpoint.h>
#include <qsgtexture.h>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

QmlMapItemStore QmlMapItemStore::qmlMapItemStore;

MapRenderFinishedEvent::MapRenderFinishedEvent()
    : QEvent(static_cast<QEvent::Type>(CustomQEvent::RenderMapFinished)) {}

float QmlMapItem::horizontalScrollSize() const
{
    if (!mapView)
    {
        return 0;
    }

    return static_cast<float>(mapView->getViewport().gameWidth()) / mapView->mapWidth();
}

float QmlMapItem::horizontalScrollPosition() const
{
    if (!mapView)
    {
        return 0;
    }

    return std::min(1 - horizontalScrollSize(), static_cast<float>(mapView->cameraPosition().x) / mapView->mapWidth());
}

float QmlMapItem::verticalScrollSize() const
{
    if (!mapView)
    {
        return 0;
    }

    return static_cast<float>(mapView->getViewport().gameHeight()) / mapView->mapHeight();
}

float QmlMapItem::verticalScrollPosition() const
{
    if (!mapView)
    {
        return 0;
    }

    return std::min(1 - verticalScrollSize(), static_cast<float>(mapView->cameraPosition().y) / mapView->mapHeight());
}

void QmlMapItem::setHorizontalScrollPosition(float value)
{
    auto x = mapView->mapWidth() * value;
    mapView->setX(x * MapTileSize);
}

void QmlMapItem::setVerticalScrollPosition(float value)
{
    auto y = mapView->mapHeight() * value;
    mapView->setY(y * MapTileSize);
}

// int QmlMapItem::mapScrollHeight()
// {
//     return mapView->mapHeight() * mapView->getZoomFactor();
// }

QmlMapItem::QmlMapItem(std::string name)
    : _name(std::move(name)), lastDrawTime(TimePoint::now())
{
    setAcceptedMouseButtons(Qt::MouseButton::AllButtons);

    setShortcut(Qt::Key_Escape, ShortcutAction::Escape);
    setShortcut(Qt::Key_Delete, ShortcutAction::Delete);
    setShortcut(Qt::ControlModifier, Qt::Key_0, ShortcutAction::ResetZoom);
    setShortcut(Qt::KeypadModifier, Qt::Key_Plus, ShortcutAction::FloorUp);
    setShortcut(Qt::KeypadModifier, Qt::Key_Minus, ShortcutAction::FloorDown);
    setShortcut(Qt::ControlModifier, Qt::Key_R, ShortcutAction::Rotate);
    setShortcut(Qt::Key_Q, ShortcutAction::LowerFloorShade);
    setShortcut(Qt::ControlModifier, Qt::Key_Z, ShortcutAction::Undo);
    setShortcut(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_Z, ShortcutAction::Redo);
    setShortcut(Qt::Key_I, ShortcutAction::BrushEyeDrop);

    mapView = std::make_unique<MapView>(std::make_unique<QmlUIUtils>(this), EditorAction::editorAction, std::make_shared<Map>());

    mapView->onViewportChanged<&QmlMapItem::onMapViewportChanged>(this);
    mapView->onDrawRequested<&QmlMapItem::mapViewDrawRequested>(this);

    drawTimer.setSingleShot(true);
    connect(&drawTimer, &QTimer::timeout, this, &QmlMapItem::delayedUpdate, Qt::DirectConnection);

    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow *win) {
        if (win)
        {
            // auto tabData = QmlMapItemStore::qmlMapItemStore.mapTabs()->getById(_id);
            // if (tabData)
            // {
            //     tabData->item = this;
            // }

            if (_id == 0)
            {
                _active = true;
            }

            auto devicePixelRatio = win->effectiveDevicePixelRatio();
            const QSize itemSize = this->size().toSize() * devicePixelRatio;

            mapView->setViewportSize(itemSize.width(), itemSize.height());

            mapView->setUiUtils(std::make_unique<QmlUIUtils>(this));

            connect(win, &QQuickWindow::beforeFrameBegin, this, &QmlMapItem::beforeRenderMap, Qt::DirectConnection);
            connect(win, &QQuickWindow::afterFrameEnd, this, &QmlMapItem::afterRenderMap, Qt::DirectConnection);

            connect(win, &QQuickWindow::beforeSynchronizing, this, &QmlMapItem::sync, Qt::DirectConnection);
            connect(win, &QQuickWindow::sceneGraphInvalidated, this, &QmlMapItem::cleanup, Qt::DirectConnection);
        }
    });

    setFlag(ItemHasContents, true);
}

void QmlMapItem::beforeRenderMap()
{
    rendering = true;
}

void QmlMapItem::afterRenderMap()
{
    rendering = false;

    QGuiApplication::postEvent(this, new MapRenderFinishedEvent(), Qt::HighEventPriority);
}

QmlMapItem::~QmlMapItem()
{
    VME_LOG_D("~QmlMapItem");
}

void QmlMapItem::setMap(std::shared_ptr<Map> &&map)
{
    if (!mapView)
    {
        mapView = std::make_unique<MapView>(std::make_unique<QmlUIUtils>(this), EditorAction::editorAction, std::move(map));

        mapView->onViewportChanged<&QmlMapItem::onMapViewportChanged>(this);
        mapView->onDrawRequested<&QmlMapItem::mapViewDrawRequested>(this);
    }
    else
    {
        mapView->setMap(std::move(map));
    }
}

void QmlMapItem::onMapViewportChanged(const Camera::Viewport & /*unused*/)
{
    emit horizontalScrollSizeChanged();
    emit horizontalScrollPositionChanged();
    emit verticalScrollSizeChanged();
    emit verticalScrollPositionChanged();
}

void QmlMapItem::sync()
{
    if (!vulkanInfo)
    {
        vulkanInfo = std::make_shared<QtVulkanInfo>(window());
        vulkanInfo->setMaxConcurrentFrameCount(window()->graphicsStateInfo().framesInFlight);
    }

    if (!mapViewTextureNode)
    {
        // TODO Use one descriptor pool per frame
        mapViewTextureNode = new MapViewTextureNode(this, mapView, vulkanInfo, width(), height());
    }

    mapViewTextureNode->sync();

    // Update window for vulkan info
    static_cast<QtVulkanInfo *>(vulkanInfo.get())->setQmlWindow(window());
}

void QmlMapItem::cleanup()
{
    VME_LOG_D("QmlMapItem::cleanup");
}

QString QmlMapItem::qStrName()
{
    return QString::fromStdString(_name);
}

void QmlMapItem::mapViewDrawRequested()
{
    delayedUpdate();
}

QSGNode *QmlMapItem::updatePaintNode(QSGNode *qsgNode, UpdatePaintNodeData * /*unused*/)
{
    auto *node = static_cast<MapViewTextureNode *>(qsgNode);

    if (!node && (width() <= 0 || height() <= 0))
        return nullptr;

    if (!node)
    {
        if (!mapViewTextureNode)
        {
            mapViewTextureNode = new MapViewTextureNode(this, mapView, vulkanInfo, width(), height());
        }

        node = mapViewTextureNode;
    }

    mapViewTextureNode->sync();

    node->setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);
    node->setFiltering(QSGTexture::Linear);
    node->setRect(0, 0, width(), height());

    window()->update(); // ensure getting to beforeRendering() at some point

    return node;
}

void QmlMapItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size())
    {
        update();
    }
}

void QmlMapItem::defer(QEvent::Type eventType, VME::MouseEvent event)
{
    if (deferredEvents.contains(eventType))
    {
        auto &deferredEvent = deferredEvents.at(eventType);
        switch (eventType)
        {
            case QEvent::MouseButtonPress:
            {
                if (event.buttons() & VME::MouseButtons::LeftButton)
                {
                    deferredEvent.event = event;
                    return;
                }
                break;
            }
            case QEvent::MouseButtonRelease:
                break;
            case QEvent::MouseMove:
                deferredEvent.event = event;
                break;
            default:
                break;
        }
        deferredEvent.event = event;
    }
    else
    {
        deferredEvents.try_emplace(eventType, DeferredEvent{event, TimePoint::now()});
    }
}

void QmlMapItem::mousePressEvent(QMouseEvent *event)
{
    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);

    if (mouseInside)
    {
        if (event->button() == Qt::MouseButton::LeftButton)
        {
            auto e = vmeMouseEvent(event);
            if (rendering)
            {
                defer(QEvent::MouseButtonPress, e);
            }
            else
            {
                mapView->mousePressEvent(e);
            }
        }
    }
}

void QmlMapItem::mouseMoveEvent(QMouseEvent *event)
{
    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);
    auto vmeEvent = vmeMouseEvent(event);

    if (rendering)
    {
        defer(QEvent::MouseMove, vmeEvent);
    }
    else
    {
        mapView->mouseMoveEvent(vmeEvent);
    }
}

void QmlMapItem::onMousePositionChanged(int x, int y, int button, int buttons, int modifiers)
{
    auto event = QMouseEvent(QEvent::Type::MouseMove,
                             QPointF(x, y),
                             mapToGlobal(QPointF(x, y)),
                             static_cast<Qt::MouseButton>(button), static_cast<Qt::MouseButtons>(buttons),
                             static_cast<Qt::KeyboardModifiers>(modifiers));

    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);

    auto vmeEvent = vmeMouseEvent(&event);
    if (rendering)
    {
        defer(QEvent::MouseMove, vmeEvent);
    }
    else
    {
        mapView->mouseMoveEvent(vmeEvent);
    }
}

void QmlMapItem::mouseReleaseEvent(QMouseEvent *event)
{
    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);

    auto e = vmeMouseEvent(event);
    if (rendering && event->button() == Qt::MouseButton::LeftButton)
    {
        defer(QEvent::MouseButtonRelease, e);
    }
    else
    {
        mapView->mouseReleaseEvent(e);
    }
}

void QmlMapItem::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Left:
            mapView->translateX(-MapTileSize);
            break;
        case Qt::Key_Right:
            mapView->translateX(MapTileSize);
            break;
        case Qt::Key_Up:
            mapView->translateY(-MapTileSize);
            break;
        case Qt::Key_Down:
            mapView->translateY(MapTileSize);
            break;
        case Qt::Key_Space:
        {
            {
                bool panning = mapView->editorAction.is<MouseAction::Pan>();
                if (panning || QGuiApplication::mouseButtons() & Qt::MouseButton::LeftButton)
                {
                    return;
                }

                QGuiApplication::setOverrideCursor(Qt::OpenHandCursor);

                if (mapView->underMouse())
                {
                    VME_LOG_D("Under mouse");
                }

                MouseAction::Pan panAction;
                mapView->editorAction.setIfUnlocked(panAction);
                mapView->requestDraw();

                // this->currentVulkanWindow()->requestActivate();
            }
        }
        default:
            break;
    }
}

void QmlMapItem::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Space:
        {
            if (!event->isAutoRepeat())
            {
                VME_LOG_D("Space released!");
                auto *pan = mapView->editorAction.as<MouseAction::Pan>();
                if (pan)
                {
                    mapView->editorAction.setPrevious();
                    QGuiApplication::restoreOverrideCursor();
                }
            }
            break;
        }
    }
}

void QmlMapItem::wheelEvent(QWheelEvent *event)
{
    // Each wheel step is reported in 1/8 degrees. 1 degree = 8 units.
    static constexpr int angleUnitsPerDegree = 8;

    // Minimum total angle delta (in angle units) before triggering a zoom.
    // 120 units = 15 degrees (i.e. 120 / 8).
    static constexpr int minScrollUnitsForZoom = 120;

    // The vertical wheel delta in 1/8 degree units.
    const int wheelDelta = event->angleDelta().y();

    scrollAngleBuffer += wheelDelta;

    if (std::abs(scrollAngleBuffer) >= minScrollUnitsForZoom)
    {
        // Convert from angle units to degrees and perform zoom.
        int zoomDelta = scrollAngleBuffer / angleUnitsPerDegree;
        this->mapView->zoom(zoomDelta);

        scrollAngleBuffer = 0;
    }
}

bool QmlMapItem::event(QEvent *event)
{
    switch (event->type())
    {
        case QEvent::MouseMove:
            break;
            // case QEvent::Enter:
            // {
            //     bool panning = mapView->editorAction.is<MouseAction::Pan>();
            //     if (panning)
            //     {
            //         setCursor(Qt::OpenHandCursor);
            //     }
            //     mapView->setUnderMouse(true);
            //     break;
            // }

            // case QEvent::DragEnter:
            //     dragEnterEvent(static_cast<QDragEnterEvent *>(event));
            //     mapView->setUnderMouse(true);
            //     mapView->dragEnterEvent();
            //     break;

            // case QEvent::DragMove:
            //     dragMoveEvent(static_cast<QDragMoveEvent *>(event));
            //     break;

        case CustomQEvent::RenderMapFinished: // NOLINT
        {
            // Run deferred events
            if (deferredEvents.contains(QEvent::MouseButtonRelease))
            {
                mapView->mouseReleaseEvent(deferredEvents.at(QEvent::MouseButtonRelease).event);
            }
            if (deferredEvents.contains(QEvent::MouseButtonPress))
            {
                mapView->mousePressEvent(deferredEvents.at(QEvent::MouseButtonPress).event);
            }
            if (deferredEvents.contains(QEvent::MouseMove))
            {
                mapView->mouseMoveEvent(deferredEvents.at(QEvent::MouseMove).event);
            }

            deferredEvents.clear();

            mapView->mapRenderFinished();

            break;
        }

        case QEvent::Leave:
        {

            mapView->setUnderMouse(false);
            break;
        }

            // case QEvent::DragLeave:
            //     dragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
            //     mapView->setUnderMouse(false);
            //     mapView->dragLeaveEvent();
            //     break;

            // case QEvent::Drop:
            //     dropEvent(static_cast<QDropEvent *>(event));
            //     break;

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
            break;
    }

    return QQuickItem::event(event);
}

void QmlMapItem::eyedrop(const Position position) const
{

    Tile *tile = mapView->map()->getTile(position);
    if (!tile)
    {
        return;
    }

    TileThing topThing = tile->getThing(Settings::BRUSH_INSERTION_OFFSET);

    rollbear::visit(
        util::overloaded{
            [this](Item *item) {
                // TODO scroll to the brush in the TilesetListView
                mapView->editorAction.setRawBrush(item->serverId());
                mapView->requestDraw();
            },
            [this](Creature *creature) {
                // TODO scroll to the brush in the TilesetListView
                mapView->editorAction.setBrush(Brushes::getCreatureBrush(creature->creatureType.id()));
                mapView->requestDraw();
            },

            [](const auto &arg) {
            }},
        topThing);
}

void QmlMapItem::shortcutPressedEvent(ShortcutAction action, QKeyEvent *event)
{
    switch (action)
    {
        case ShortcutAction::Escape:
            mapView->escapeEvent();
            break;
        case ShortcutAction::Delete:
            mapView->deleteSelectedThings();
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
        case ShortcutAction::Undo:
            mapView->undo();
            break;
        case ShortcutAction::Redo:
            mapView->redo();
            break;
        case ShortcutAction::BrushEyeDrop:
        {
            auto pos = mapFromGlobal(QCursor::pos());
            auto screenPos = ScreenPosition(pos.x(), pos.y());

            Position mapPos = screenPos.toPos(*mapView);

            eyedrop(mapPos);
            break;
        }
        case ShortcutAction::Rotate:
            mapView->rotateBrush();
            break;
    }
}

void QmlMapItem::setFocus(bool focus)
{
    _focused = focus;
}

void QmlMapItem::setActive(bool active)
{
    _active = active;
}

void QmlMapItem::setShortcut(Qt::KeyboardModifiers modifiers, Qt::Key key, ShortcutAction shortcut)
{
    int combination = QKeyCombination(modifiers, key).toCombined();
    shortcuts.emplace(combination, shortcut);
    shortcutActionToKeyCombination.emplace(shortcut, combination);
}

void QmlMapItem::setShortcut(Qt::Key key, ShortcutAction shortcut)
{
    int combination = QKeyCombination(key).toCombined();
    shortcuts.emplace(combination, shortcut);
    shortcutActionToKeyCombination.emplace(shortcut, combination);
}

std::optional<ShortcutAction> QmlMapItem::getShortcutAction(QKeyEvent *event) const
{
    auto shortcut = shortcuts.find(event->key() | event->modifiers());

    return shortcut != shortcuts.end() ? std::optional<ShortcutAction>(shortcut->second) : std::nullopt;
}

bool QmlMapItem::containsMouse() const
{
    QSizeF selfSize = size();
    auto mousePos = mapFromGlobal(QCursor::pos());

    bool result = (0 <= mousePos.x() && mousePos.x() <= selfSize.width()) && (0 <= mousePos.y() && mousePos.y() <= selfSize.height());

    return result;
}

void QmlMapItem::invalidateSceneGraph() // called on the render thread when the scenegraph is invalidated
{
    if (mapViewTextureNode)
    {
        delete mapViewTextureNode;
        mapViewTextureNode = nullptr;
    }
}

void QmlMapItem::releaseResources() // called on the gui thread if the item is removed from scene
{
    if (mapViewTextureNode)
    {
        delete mapViewTextureNode;
        mapViewTextureNode = nullptr;
    }
}

void QmlMapItem::setEntryId(int id)
{
    if (_id != id)
    {
        _id = id;
        emit entryIdChanged();
    }
}

void QmlMapItem::scheduleDraw(int millis)
{
    if (millis == 0)
    {
        delayedUpdate();
    }
    else
    {
        QTimer::singleShot(millis, this, [this]() {
            delayedUpdate();
        });
    }
}

void QmlMapItem::delayedUpdate()
{
    auto elapsedMillis = lastDrawTime.elapsedMillis();
    if (elapsedMillis < minTimePerFrameMs)
    {
        if (!drawTimer.isActive())
        {
            drawTimer.start(minTimePerFrameMs - elapsedMillis);
        }
        return;
    }

    lastDrawTime = TimePoint::now();
    update();
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>MapViewTextureNode>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

MapViewTextureNode::MapViewTextureNode(QmlMapItem *item, const std::shared_ptr<MapView> &mapView, std::shared_ptr<VulkanInfo> &vulkanInfo, uint32_t width, uint32_t height)
    : m_item(item),
      mapView(mapView),
      vulkanInfo(vulkanInfo)
{
    m_window = m_item->window();

    renderCoordinator = std::make_unique<RenderCoordinator>(vulkanInfo, mapView, width, height);

    connect(m_window, &QQuickWindow::beforeRendering, this, &MapViewTextureNode::render);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != devicePixelRatio)
            m_item->update();
    });

    qtSceneGraphTextures.fill(nullptr);
}

MapViewTextureNode::~MapViewTextureNode()
{
    // VME_LOG_D("~MapViewTextureNode()");
    delete texture();
}

QSGTexture *MapViewTextureNode::texture() const
{
    return QSGSimpleTextureNode::texture();
}

void MapViewTextureNode::freeTexture()
{
    renderCoordinator->destroyResources();
}

void MapViewTextureNode::syncTexture()
{
    int currentFrameSlot = m_window->graphicsStateInfo().currentFrameSlot;

    // The native object is wrapped, but not owned, by the resulting QSGTexture.
    // The caller of the function is responsible for deleting the returned QSGTexture,
    // but that will not destroy the underlying native object.
    // https://doc.qt.io/qt-6/qnativeinterface-qsgvulkantexture.html#fromNative
    QSGTexture *texture = QNativeInterface::QSGVulkanTexture::fromNative(
        renderCoordinator->frameResources[currentFrameSlot].mapAttachment.image,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        m_window,
        textureSize);

    // Make sure to delete the old texture before replacing it.
    if (qtSceneGraphTextures[currentFrameSlot])
    {
        delete qtSceneGraphTextures[currentFrameSlot];
    }

    VME_LOG_D("Syncing texture for frame slot " << currentFrameSlot
                                                << " with size: " << textureSize.width() << "x" << textureSize.height());
    qtSceneGraphTextures[currentFrameSlot] = texture;

    setTexture(texture);
}

void MapViewTextureNode::sync()
{
    uint32_t frameIndex = m_window->graphicsStateInfo().currentFrameSlot;

    // Ensure that the texture is set to the current frame's texture.
    auto *frameTexture = qtSceneGraphTextures[frameIndex];
    if (texture() != qtSceneGraphTextures[frameIndex] && frameTexture)
    {
        setTexture(frameTexture);
        VME_LOG_D("MapViewTextureNode::sync: Texture set for frame index " << frameIndex);
    }

    devicePixelRatio = m_window->effectiveDevicePixelRatio();
    const QSize newSize = m_item->size().toSize() * devicePixelRatio;
    auto MIN_RENDERING_SIZE = QSize(100, 100);
    if (newSize.width() < MIN_RENDERING_SIZE.width() || newSize.height() < MIN_RENDERING_SIZE.height())
    {
        // TODO Investigate why we are getting loads of invalid sizes (~20x20 pixels)
        // VME_LOG_D("MapViewTextureNode::sync: Invalid size, skipping sync.");
        return;
    }

    bool needsNewTexture = !texture() || (textureSize != newSize);

    if (needsNewTexture)
    {

        if (newSize != textureSize)
        {
            VME_LOG_D("New size: [" << newSize.width() << " x " << newSize.height() << "] (frameIndex: " << frameIndex << ")");
            renderCoordinator->resize(newSize.width(), newSize.height());
            if (auto wMapView = mapView.lock())
            {
                wMapView->setViewportSize(newSize.width(), newSize.height());
            }
            textureSize = newSize;
        }

        syncTexture();
    }
}

void MapViewTextureNode::render()
{
    if (!m_item->isActive())
    {
        return;
    }

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    auto *resource = renderInterface->getResource(m_window, QSGRendererInterface::CommandListResource);
    if (!resource)
    {
        VME_LOG_D("No QSGRendererInterface::CommandListResource yet.");
        return;
    }

    VkCommandBuffer commandBuffer = *reinterpret_cast<VkCommandBuffer *>(resource);
    Q_ASSERT(commandBuffer);

    auto currentFrameSlot = m_window->graphicsStateInfo().currentFrameSlot;
    // VME_LOG_D("Current frame slot: " << currentFrameSlot);

    renderCoordinator->recordFrame(commandBuffer, currentFrameSlot);

    if (Settings::RENDER_ANIMATIONS && renderCoordinator->containsAnimation())
    {
        // TODO Allow control of update frequence for animations
        constexpr int MILLIS_PER_FRAME = 1000 / 60;
        m_item->scheduleDraw(MILLIS_PER_FRAME);
    }

    // [QT doc]
    // Memory barrier before the texture can be used as a source.
    // Since we are not using a sub-pass, we have to do this explicitly.

    VkImageMemoryBarrier imageTransitionBarrier = {};
    imageTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageTransitionBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageTransitionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageTransitionBarrier.image = renderCoordinator->frameResources[currentFrameSlot].mapAttachment.image;
    imageTransitionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageTransitionBarrier.subresourceRange.levelCount = imageTransitionBarrier.subresourceRange.layerCount = 1;

    vulkanInfo->vkCmdPipelineBarrier(commandBuffer,
                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                     VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                     0, 0, nullptr, 0, nullptr,
                                     1, &imageTransitionBarrier);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>MapTabListModel>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

MapTabListModel::MapTabListModel(QObject *parent)
    : QAbstractListModel(parent) {}

void MapTabListModel::addTab(const std::string &tabName)
{
    auto modelSize = static_cast<int>(_data.size());

    uint32_t id = _nextId++;

    beginInsertRows(QModelIndex(), modelSize, modelSize);
    _data.push_back(TabData{.name = tabName, .id = id, .item = nullptr});
    endInsertRows();

    VME_LOG_D("MapTabListModel::addTab: tab " << tabName << " added with id " << id << ". Size: " << size());

    // emit sizeChanged(size());
}

void MapTabListModel::removeTab(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    _data.erase(_data.begin() + index);
    endRemoveRows();
    // emit sizeChanged(size());
}

void MapTabListModel::removeTabById(int id)
{
    auto found = std::find_if(_data.cbegin(), _data.cend(), [id](const TabData &tabData) { return tabData.id == id; });
    if (found != _data.cend())
    {
        auto index = std::distance(_data.cbegin(), found);
        removeTab(index);
    }
    else
    {
        VME_LOG_ERROR("MapTabListModel::removeTab: tab with id " << id << " not found");
    }
}

QVariant MapTabListModel::data(const QModelIndex &modelIndex, int role) const
{
    auto index = modelIndex.row();
    if (index < 0 || index >= rowCount())
        return {};

    if (role == to_underlying(Role::QmlMapItem))
    {
        return QVariant::fromValue(QString::fromStdString(_data.at(index).name));
    }
    else if (role == to_underlying(Role::EntryId))
    {
        return QVariant::fromValue(static_cast<int>(_data.at(index).id));
    }

    return {};
}

MapTabListModel::TabData *MapTabListModel::getById(int id)
{
    auto found = std::find_if(_data.begin(), _data.end(), [id](const TabData &tabData) { return tabData.id == id; });

    return found != _data.end() ? &*found : nullptr;
}

QHash<int, QByteArray> MapTabListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[to_underlying(Role::QmlMapItem)] = "item";
    roles[to_underlying(Role::EntryId)] = "theId";

    return roles;
}

int MapTabListModel::size() const
{
    return rowCount();
}

bool MapTabListModel::empty() const
{
    return size() == 0;
}

int MapTabListModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(_data.size());
}

void MapTabListModel::setInstance(int index, QmlMapItem *instance)
{
    _data.at(index).item = instance;
}

// #include "moc_qml_map_item.cpp"