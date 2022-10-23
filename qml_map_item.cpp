#include "qml_map_item.h"

#include "common/map_renderer.h"
#include "src/qml_ui_utils.h"

//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>MockUIUtils2>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>

std::shared_ptr<Map> testMap2()
{
    std::shared_ptr<Map> map = std::make_shared<Map>();

    for (int x = 5; x < 20; ++x)
    {
        for (int y = 5; y < 20; ++y)
        {
            Item gold(2148);
            Item something(2152);
            gold.setCount(25);
            // map->addItem(Position(x, y, 7), 4526);
            map->addItem(Position(x, y, 7), std::move(gold));
            map->addItem(Position(x, y, 7), std::move(something));
        }
    }

    // {
    //     Item gold(2148);
    //     Item platinum(2152);
    //     gold.setCount(25);
    //     map->addItem(Position(7, 7, 7), 4526);
    //     map->addItem(Position(7, 7, 7), std::move(gold));
    //     map->addItem(Position(7, 7, 7), std::move(platinum));
    // }
    // {
    //     Item gold(2148);
    //     gold.setCount(25);
    //     map->addItem(Position(8, 8, 7), 4526);
    //     map->addItem(Position(8, 8, 7), std::move(gold));
    // }
    // {
    //     map->addItem(Position(9, 9, 7), 4526);
    // }

    return map;
}

QmlMapItem::QmlMapItem()
{
    setAcceptedMouseButtons(Qt::MouseButton::AllButtons);
    // VME_LOG_D("QmlMapItem::QmlMapItem");
    connect(this, &QQuickItem::windowChanged, this, [this]() {
        connect(window(), &QQuickWindow::sceneGraphInitialized, this, [this]() {
            if (!m_initialized)
            {
                initialize();
            }
        });
    });

    setFlag(ItemHasContents, true);
}

QmlMapItem::~QmlMapItem()
{
    // VME_LOG_D("~QmlMapItem");
}

void QmlMapItem::initialize()
{
    // VME_LOG_D("QmlMapItem::initialize");
    auto qWindow = window();
    if (qWindow)
    {
        vulkanInfo = std::make_shared<QtVulkanInfo>(window());
        mapView = std::make_shared<MapView>(std::make_unique<QmlUIUtils>(), action, testMap2());
        mapRenderer = std::make_shared<MapRenderer>(vulkanInfo, mapView);

        mapView->onDrawRequested<&QmlMapItem::mapViewDrawRequested>(this);

        mapView->setX(0);
        mapView->setY(0);
        mapView->setViewportSize(1920, 1080);

        const QQuickWindow::GraphicsStateInfo &stateInfo(qWindow->graphicsStateInfo());

        vulkanInfo->setMaxConcurrentFrameCount(stateInfo.framesInFlight);

        VkSurfaceKHR surface = qWindow->vulkanInstance()->surfaceForWindow(qWindow);

        QSize size = qWindow->size() * qWindow->devicePixelRatio();
        uint32_t width = static_cast<uint32_t>(size.width());
        uint32_t height = static_cast<uint32_t>(size.height());

        mapRenderer->initResources(surface, width, height);
        m_initialized = true;
    }
}

void QmlMapItem::mapViewDrawRequested()
{
    update();
}

QSGNode *QmlMapItem::updatePaintNode(QSGNode *qsgNode, UpdatePaintNodeData *)
{
    // VME_LOG_D("QmlMapItem::updatePaintNode");
    MapTextureNode *textureNode = static_cast<MapTextureNode *>(qsgNode);

    if (!m_initialized)
    {
        initialize();
    }

    if ((!textureNode && (width() <= 0 || height() <= 0)) || !m_initialized)
    {
        return nullptr;
    }

    if (!textureNode)
    {
        mapTextureNode = new MapTextureNode(this, mapView, vulkanInfo, mapRenderer, width(), height());
    }

    mapTextureNode->sync();

    mapTextureNode->setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);
    mapTextureNode->setFiltering(QSGTexture::Linear);
    mapTextureNode->setRect(0, 0, width(), height());

    window()->update(); // ensure getting to beforeRendering() at some point

    return mapTextureNode;
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

    if (event->button() == Qt::MouseButton::LeftButton)
    {
        auto e = vmeMouseEvent(event);
        mapView->mousePressEvent(e);
    }
}

void QmlMapItem::mouseMoveEvent(QMouseEvent *event)
{
    bool mouseInside = containsMouse();
    mapView->setUnderMouse(mouseInside);

    auto e = vmeMouseEvent(event);
    mapView->mouseMoveEvent(e);
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
    VME_LOG_D(event->key());
}

bool QmlMapItem::containsMouse() const
{
    QSizeF selfSize = size();
    auto mousePos = mapFromGlobal(QCursor::pos());

    bool result = (0 <= mousePos.x() && mousePos.x() <= selfSize.width()) && (0 <= mousePos.y() && mousePos.y() <= selfSize.height());
    VME_LOG_D("containsMouse: " << result);

    return result;
}

void QmlMapItem::invalidateSceneGraph() // called on the render thread when the scenegraph is invalidated
{
    // VME_LOG_D("QmlMapItem::invalidateSceneGraph");
    releaseResources();
}

void QmlMapItem::releaseResources() // called on the gui thread if the item is removed from scene
{
    // VME_LOG_D("QmlMapItem::releaseResources");
    if (mapTextureNode)
    {
        mapTextureNode->freeTexture();
    }

    mapTextureNode = nullptr;

    mapView = std::make_shared<MapView>(std::make_unique<QmlUIUtils>(), action, testMap2());
    mapRenderer.reset();
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>MapTextureNode>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

MapTextureNode::MapTextureNode(QQuickItem *item, std::shared_ptr<MapView> mapView, std::shared_ptr<VulkanInfo> &vulkanInfo, std::shared_ptr<MapRenderer> renderer, uint32_t width, uint32_t height)
    : m_item(item), mapView(mapView), vulkanInfo(vulkanInfo), mapRenderer(renderer), screenTexture(vulkanInfo)
{
    // VME_LOG_D("MapTextureNode::MapTextureNode");
    m_window = m_item->window();
    connect(m_window, &QQuickWindow::beforeRendering, this, &MapTextureNode::render);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != m_dpr)
            m_item->update();
    });
}

MapTextureNode::~MapTextureNode()
{
    // VME_LOG_D("~MapTextureNode()");
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
    // VME_LOG_D("MapTextureNode::sync");
    m_dpr = m_window->effectiveDevicePixelRatio();
    const QSize newSize = m_window->size() * m_dpr;

    bool needsNewTexture = false;

    if (!texture())
    {
        needsNewTexture = true;
    }

    if (newSize != m_size)
    {
        needsNewTexture = true;
        m_size = newSize;
    }

    if (needsNewTexture)
    {
        if (auto renderer = mapRenderer.lock())
        {
            screenTexture.recreate(renderer->getRenderPass(), m_size.width(), m_size.height());

            VkImage texture = screenTexture.texture();
            textureWrapper = std::unique_ptr<QSGTexture>(QNativeInterface::QSGVulkanTexture::fromNative(texture,
                                                                                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                                                        m_window,
                                                                                                        m_size));
            // VME_LOG_D("[MapTextureNode::sync] setTexture");

            setTexture(textureWrapper.get());
            // Q_ASSERT(wrapper->nativeInterface<QNativeInterface::QSGVulkanTexture>()->nativeImage() == texture);
        }
    }
}

void MapTextureNode::render()
{
    QSGRendererInterface *renderInterface = m_window->rendererInterface();
    VkCommandBuffer commandBuffer = *reinterpret_cast<VkCommandBuffer *>(
        renderInterface->getResource(m_window, QSGRendererInterface::CommandListResource));
    Q_ASSERT(commandBuffer);

    auto currentFrameSlot = m_window->graphicsStateInfo().currentFrameSlot;

    if (auto renderer = mapRenderer.lock())
    {
        renderer->setCurrentFrame(currentFrameSlot);

        auto frame = renderer->currentFrame();
        frame->currentFrameIndex = currentFrameSlot;
        frame->commandBuffer = commandBuffer;

        if (auto wMapView = mapView.lock())
        {
            frame->mouseAction = wMapView->editorAction.action();
        }

        renderer->render(screenTexture.vkFrameBuffer());
    }
}
