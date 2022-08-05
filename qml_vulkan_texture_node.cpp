#include "qml_vulkan_texture_node.h"

#include "common/map_renderer.h"

class MockUIUtils2 : public UIUtils
{
  public:
    MockUIUtils2() {}
    ScreenPosition mouseScreenPosInView() override;

    double screenDevicePixelRatio() override;
    double windowDevicePixelRatio() override;

    VME::ModifierKeys modifiers() const override;

    void waitForDraw(std::function<void()> f) override;

  private:
};

// UI Utils

ScreenPosition MockUIUtils2::mouseScreenPosInView()
{
    return ScreenPosition(0, 0);
}

double MockUIUtils2::screenDevicePixelRatio()
{
    return 1.0;
}

double MockUIUtils2::windowDevicePixelRatio()
{
    return 1.0;
}

VME::ModifierKeys MockUIUtils2::modifiers() const
{
    return VME::ModifierKeys::None;
}

void MockUIUtils2::waitForDraw(std::function<void()> f)
{
    // No-op
}

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

CustomTextureItem::CustomTextureItem()
{
    // VME_LOG_D("CustomTextureItem::CustomTextureItem");
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

void CustomTextureItem::initialize()
{
    // VME_LOG_D("CustomTextureItem::initialize");
    auto qWindow = window();
    if (qWindow)
    {
        vulkanInfo = std::make_unique<QtVulkanInfo>(window());
        mapView = std::make_unique<MapView>(std::make_unique<MockUIUtils2>(), action, testMap2());
        mapRenderer = std::make_unique<MapRenderer>(*vulkanInfo, mapView.get());

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

QSGNode *CustomTextureItem::updatePaintNode(QSGNode *node, UpdatePaintNodeData *)
{
    // VME_LOG_D("CustomTextureItem::updatePaintNode");
    CustomTextureNode *textureNode = static_cast<CustomTextureNode *>(node);

    if (!m_initialized)
    {
        initialize();
    }

    if (!textureNode && (width() <= 0 || height() <= 0))
    {
        return nullptr;
    }

    if (!textureNode)
    {
        m_node = new CustomTextureNode(this, vulkanInfo.get(), mapRenderer.get(), width(), height());
        textureNode = m_node;
    }

    m_node->sync();

    textureNode->setTextureCoordinatesTransform(QSGSimpleTextureNode::NoTransform);
    textureNode->setFiltering(QSGTexture::Linear);
    textureNode->setRect(0, 0, width(), height());

    window()->update(); // ensure getting to beforeRendering() at some point

    return textureNode;
}

void CustomTextureItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    if (newGeometry.size() != oldGeometry.size())
    {
        update();
    }
}

void CustomTextureItem::invalidateSceneGraph() // called on the render thread when the scenegraph is invalidated
{
    m_node = nullptr;
}

void CustomTextureItem::releaseResources() // called on the gui thread if the item is removed from scene
{
    m_node = nullptr;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>CustomTextureNode>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

CustomTextureNode::CustomTextureNode(QQuickItem *item, QtVulkanInfo *vulkanInfo, MapRenderer *renderer, uint32_t width, uint32_t height)
    : m_item(item), vulkanInfo(vulkanInfo), mapRenderer(renderer)
{
    // VME_LOG_D("CustomTextureNode::CustomTextureNode");
    m_window = m_item->window();
    connect(m_window, &QQuickWindow::beforeRendering, this, &CustomTextureNode::render);
    connect(m_window, &QQuickWindow::screenChanged, this, [this]() {
        if (m_window->effectiveDevicePixelRatio() != m_dpr)
            m_item->update();
    });

    initialize();
}

void CustomTextureNode::initialize()
{
}

QSGTexture *CustomTextureNode::texture() const
{
    return QSGSimpleTextureNode::texture();
}

void CustomTextureNode::sync()
{
    // VME_LOG_D("CustomTextureNode::sync");
    m_dpr = m_window->effectiveDevicePixelRatio();
    const QSize newSize = m_window->size() * m_dpr;
    bool needsNew = false;

    if (!texture())
    {
        needsNew = true;
    }

    if (newSize != m_size)
    {
        needsNew = true;
        m_size = newSize;
    }

    if (needsNew)
    {
        // Maybe unnecessary
        vulkanInfo->update();
        screenTexture.recreate(vulkanInfo, mapRenderer->getRenderPass(), m_size.width(), m_size.height());

        VkImage texture = screenTexture.texture();
        QSGTexture *wrapper = QNativeInterface::QSGVulkanTexture::fromNative(texture,
                                                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                             m_window,
                                                                             m_size);
        // VME_LOG_D("[CustomTextureNode::sync] setTexture");

        setTexture(wrapper);
        // Q_ASSERT(wrapper->nativeInterface<QNativeInterface::QSGVulkanTexture>()->nativeImage() == texture);
    }
}

void CustomTextureNode::render()
{
    QSGRendererInterface *rif = m_window->rendererInterface();
    VkCommandBuffer cb = *reinterpret_cast<VkCommandBuffer *>(
        rif->getResource(m_window, QSGRendererInterface::CommandListResource));
    Q_ASSERT(cb);

    auto currentFrameSlot = m_window->graphicsStateInfo().currentFrameSlot;
    mapRenderer->setCurrentFrame(currentFrameSlot);
    auto frame = mapRenderer->currentFrame();
    frame->currentFrameIndex = currentFrameSlot;
    frame->commandBuffer = cb;

    mapRenderer->render(screenTexture.vkFrameBuffer());
}
