#include "qml_map_item.h"

#include <QGuiApplication>

#include "core/brushes/brush.h"
#include "core/brushes/creature_brush.h"
#include "core/const.h"
#include "core/map_renderer.h"
#include "core/vendor/rollbear-visit/visit.hpp"
#include "qml_ui_utils.h"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

QmlMapItemStore QmlMapItemStore::qmlMapItemStore;

std::shared_ptr<Map> testMap2()
{
    std::shared_ptr<Map> map = std::make_shared<Map>();

    for (int x = 5; x < 8; ++x)
    {
        for (int y = 5; y < 8; ++y)
        {
            Item grass(4526 + x - 5);
            map->addItem(Position(x, y, 7), std::move(grass));
        }
    }

    return map;
}

float QmlMapItem::horizontalScrollSize()
{
    if (!mapView)
    {
        return 0;
    }

    return static_cast<float>(mapView->getViewport().gameWidth()) / mapView->mapWidth();
}

float QmlMapItem::horizontalScrollPosition()
{
    if (!mapView)
    {
        return 0;
    }

    return std::min(1 - horizontalScrollSize(), static_cast<float>(mapView->cameraPosition().x) / mapView->mapWidth());
}

float QmlMapItem::verticalScrollSize()
{
    if (!mapView)
    {
        return 0;
    }

    return static_cast<float>(mapView->getViewport().gameHeight()) / mapView->mapHeight();
}

float QmlMapItem::verticalScrollPosition()
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
    : _name(name)
{
    setAcceptedMouseButtons(Qt::MouseButton::AllButtons);

    setShortcut(Qt::Key_Escape, ShortcutAction::Escape);
    setShortcut(Qt::Key_Delete, ShortcutAction::Delete);
    setShortcut(Qt::ControlModifier, Qt::Key_0, ShortcutAction::ResetZoom);
    setShortcut(Qt::KeypadModifier, Qt::Key_Plus, ShortcutAction::FloorUp);
    setShortcut(Qt::KeypadModifier, Qt::Key_Minus, ShortcutAction::FloorDown);
    setShortcut(Qt::Key_Q, ShortcutAction::LowerFloorShade);
    setShortcut(Qt::ControlModifier, Qt::Key_Z, ShortcutAction::Undo);
    setShortcut(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_Z, ShortcutAction::Redo);
    setShortcut(Qt::Key_I, ShortcutAction::BrushEyeDrop);

    connect(this, &QQuickItem::windowChanged, this, [this](QQuickWindow *win) {
        if (win)
        {
            QmlMapItemStore::qmlMapItemStore.mapTabs()->getById(_id)->item = this;

            if (_id == 0)
            {
                _active = true;
            }

            if (!mapView)
            {
                mapView = std::make_unique<MapView>(std::make_unique<QmlUIUtils>(this), EditorAction::editorAction, testMap2());

                mapView->onViewportChanged<&QmlMapItem::onMapViewportChanged>(this);
                mapView->onDrawRequested<&QmlMapItem::mapViewDrawRequested>(this);

                mapView->setX(0);
                mapView->setY(0);

                auto devicePixelRatio = win->effectiveDevicePixelRatio();
                const QSize itemSize = this->size().toSize() * devicePixelRatio;

                mapView->setViewportSize(itemSize.width(), itemSize.height());
            }

            mapView->setUiUtils(std::make_unique<QmlUIUtils>(this));

            connect(win, &QQuickWindow::beforeSynchronizing, this, &QmlMapItem::sync, Qt::DirectConnection);
            connect(win, &QQuickWindow::sceneGraphInvalidated, this, &QmlMapItem::cleanup, Qt::DirectConnection);
        }
    });

    setFlag(ItemHasContents, true);
}

QmlMapItem::~QmlMapItem()
{
    VME_LOG_D("~QmlMapItem");
}

void QmlMapItem::onMapViewportChanged(const Camera::Viewport &)
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

    if (!textureNode)
    {
        textureNode = new MapTextureNode(this, mapView, vulkanInfo, width(), height());
    }

    textureNode->sync();

    // Update window for vulkan info
    static_cast<QtVulkanInfo *>(vulkanInfo.get())->setQmlWindow(window());
}

void QmlMapItem::cleanup()
{
}

QString QmlMapItem::qStrName()
{
    return QString::fromStdString(_name);
}
MapTabListModel::TabData *MapTabListModel::getById(int id)
{
    auto found = std::find_if(_data.begin(), _data.end(), [id](const TabData &tabData) { return tabData.id == id; });

    return found != _data.end() ? &*found : nullptr;
}

void QmlMapItem::mapViewDrawRequested()
{
    update();
}

QSGNode *QmlMapItem::updatePaintNode(QSGNode *qsgNode, UpdatePaintNodeData *)
{
    MapTextureNode *node = static_cast<MapTextureNode *>(qsgNode);

    if (!node && (width() <= 0 || height() <= 0))
        return nullptr;

    if (!node)
    {
        if (!textureNode)
        {
            textureNode = new MapTextureNode(this, mapView, vulkanInfo, width(), height());
        }

        node = textureNode;
    }

    textureNode->sync();

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

void QmlMapItem::mousePressEvent(QMouseEvent *event)
{
    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);

    if (mouseInside)
    {
        if (event->button() == Qt::MouseButton::LeftButton)
        {
            auto e = vmeMouseEvent(event);
            mapView->mousePressEvent(e);
        }
    }
}

void QmlMapItem::mouseMoveEvent(QMouseEvent *event)
{
    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);

    mapView->mouseMoveEvent(vmeMouseEvent(event));
}

void QmlMapItem::onMousePositionChanged(int x, int y, int button, int buttons, int modifiers)
{
    auto event = QMouseEvent(
        QEvent::Type::MouseMove,
        mapToGlobal(QPointF(x, y)),
        static_cast<Qt::MouseButton>(button), static_cast<Qt::MouseButtons>(buttons),
        static_cast<Qt::KeyboardModifiers>(modifiers));

    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);

    auto vmeEvent = vmeMouseEvent(&event);
    mapView->mouseMoveEvent(vmeEvent);
}

void QmlMapItem::mouseReleaseEvent(QMouseEvent *event)
{
    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);

    auto e = vmeMouseEvent(event);
    mapView->mouseReleaseEvent(e);
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
                auto pan = mapView->editorAction.as<MouseAction::Pan>();
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
        this->mapView->zoom(scrollAngleBuffer / 8);
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
    TileThing topThing = mapView->map()->getTopThing(position);

    rollbear::visit(
        util::overloaded{
            [this](Item *item) {
                // TODO scroll to the brush in the TilesetListView
                mapView->editorAction.setRawBrush(item->serverId());
                mapView->requestDraw();
            },
            [this](Creature *creature) {
                // TODO scroll to the brush in the TilesetListView
                mapView->editorAction.setBrush(Brush::getCreatureBrush(creature->creatureType.id()));
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

            Position mapPos = screenPos.toPos(*mapView.get());

            eyedrop(mapPos);
            break;
        }
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
    textureNode = nullptr;
}

void QmlMapItem::releaseResources() // called on the gui thread if the item is removed from scene
{
    textureNode = nullptr;
}

void QmlMapItem::setEntryId(int id)
{
    if (_id != id)
    {
        _id = id;
        emit entryIdChanged();
    }
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>MapTextureNode>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

MapTextureNode::MapTextureNode(QmlMapItem *item, std::shared_ptr<MapView> mapView, std::shared_ptr<VulkanInfo> &vulkanInfo, uint32_t width, uint32_t height)
    : m_item(item), mapView(mapView), vulkanInfo(vulkanInfo), mapRenderer(std::make_unique<MapRenderer>(vulkanInfo, mapView)), screenTexture(vulkanInfo)
{
    m_window = m_item->window();

    mapRenderer->initResources();

    connect(m_window, &QQuickWindow::beforeRendering, this, &MapTextureNode::render);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != devicePixelRatio)
            m_item->update();
    });
}

MapTextureNode::~MapTextureNode()
{
    // VME_LOG_D("~MapTextureNode()");
    delete texture();
}

void MapTextureNode::frameStart()
{
    // VME_LOG_D("MapTextureNode::frameStart");
    QSGRendererInterface *rif = m_window->rendererInterface();

    // We are not prepared for anything other than running with the RHI and its Vulkan backend.
    Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::Vulkan);
}

QSGTexture *MapTextureNode::texture() const
{
    return QSGSimpleTextureNode::texture();
}

void MapTextureNode::freeTexture()
{
    screenTexture.releaseResources();
}

void MapTextureNode::sync()
{
    devicePixelRatio = m_window->effectiveDevicePixelRatio();
    const QSize newSize = m_item->size().toSize() * devicePixelRatio;

    bool needsNewTexture = false;

    if (!texture())
    {
        needsNewTexture = true;
    }

    if (newSize != textureSize)
    {
        needsNewTexture = true;
        textureSize = newSize;
    }

    if (needsNewTexture)
    {
        delete texture();
        screenTexture.recreate(mapRenderer->getRenderPass(), textureSize.width(), textureSize.height());

        VkImage texture = screenTexture.texture();
        QSGTexture *wrapper = QNativeInterface::QSGVulkanTexture::fromNative(texture,
                                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                             m_window,
                                                                             textureSize);
        setTexture(wrapper);

        if (auto wMapView = mapView.lock())
        {
            wMapView->setViewportSize(newSize.width(), newSize.height());
        }
    }
}

void MapTextureNode::render()
{
    if (!m_item->isActive())
    {
        return;
    }

    VkFramebuffer frameBuffer = screenTexture.vkFrameBuffer();

    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    auto resource = renderInterface->getResource(m_window, QSGRendererInterface::CommandListResource);
    VkCommandBuffer commandBuffer = *reinterpret_cast<VkCommandBuffer *>(resource);
    Q_ASSERT(commandBuffer);

    auto currentFrameSlot = m_window->graphicsStateInfo().currentFrameSlot;

    mapRenderer->setCurrentFrame(currentFrameSlot);

    auto frame = mapRenderer->currentFrame();
    frame->currentFrameIndex = currentFrameSlot;
    frame->commandBuffer = commandBuffer;

    if (auto wMapView = mapView.lock())
    {
        frame->mouseAction = wMapView->editorAction.action();
    }

    mapRenderer->render(frameBuffer, util::Size(textureSize.width(), textureSize.height()));

    // [QT doc]
    // Memory barrier before the texture can be used as a source.
    // Since we are not using a sub-pass, we have to do this explicitly.

    VkImageMemoryBarrier imageTransitionBarrier = {};
    imageTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageTransitionBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageTransitionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageTransitionBarrier.image = screenTexture.texture();
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

void MapTabListModel::addTab(std::string tabName)
{
    auto modelSize = static_cast<int>(_data.size());

    uint32_t id = _nextId++;

    beginInsertRows(QModelIndex(), modelSize, modelSize);
    _data.push_back(TabData{tabName, id, nullptr});
    endInsertRows();

    emit sizeChanged(size());
}

void MapTabListModel::removeTab(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    _data.erase(_data.begin() + index);
    endRemoveRows();
    emit sizeChanged(size());
}

QVariant MapTabListModel::data(const QModelIndex &modelIndex, int role) const
{
    auto index = modelIndex.row();
    if (index < 0 || index >= rowCount())
        return QVariant();

    if (role == to_underlying(Role::QmlMapItem))
    {
        return QVariant::fromValue(QString::fromStdString(_data.at(index).name));
    }
    else if (role == to_underlying(Role::EntryId))
    {
        return QVariant::fromValue(static_cast<int>(_data.at(index).id));
    }

    return QVariant();
}

QHash<int, QByteArray> MapTabListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[to_underlying(Role::QmlMapItem)] = "item";
    roles[to_underlying(Role::EntryId)] = "theId";

    return roles;
}

int MapTabListModel::size()
{
    return rowCount();
}

bool MapTabListModel::empty()
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

#include "moc_qml_map_item.cpp"