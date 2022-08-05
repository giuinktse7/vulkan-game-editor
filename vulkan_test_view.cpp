#include "vulkan_test_view.h"

// #include <QRhiSwapChain>
#include <QtQuick/qquickwindow.h>
#include <memory>

MapViewItem::MapViewItem()
{
    connect(this, &QQuickItem::windowChanged, this, &MapViewItem::handleWindowChanged);
}

void MapViewItem::setT(qreal t)
{
    if (t == m_t)
        return;
    m_t = t;
    emit tChanged();
    if (window())
        window()->update();
}

void MapViewItem::handleWindowChanged(QQuickWindow *win)
{
    if (win)
    {
        connect(win, &QQuickWindow::beforeSynchronizing, this, &MapViewItem::sync, Qt::DirectConnection);
        connect(win, &QQuickWindow::sceneGraphInvalidated, this, &MapViewItem::cleanup, Qt::DirectConnection);

        // Ensure we start with cleared to black. The squircle's blend mode relies on this.
        win->setColor(Qt::black);
    }
}

// The safe way to release custom graphics resources is to both connect to
// sceneGraphInvalidated() and implement releaseResources(). To support
// threaded render loops the latter performs the MapViewItemRenderer destruction
// via scheduleRenderJob(). Note that the MapViewItem may be gone by the time
// the QRunnable is invoked.

void MapViewItem::cleanup()
{
    delete m_renderer;
    m_renderer = nullptr;
}

class CleanupJob : public QRunnable
{
  public:
    CleanupJob(MapViewItemRenderer *renderer)
        : m_renderer(renderer) {}
    void run() override
    {
        delete m_renderer;
    }

  private:
    MapViewItemRenderer *m_renderer;
};

void MapViewItem::releaseResources()
{
    window()->scheduleRenderJob(new CleanupJob(m_renderer), QQuickWindow::BeforeSynchronizingStage);
    m_renderer = nullptr;
}

std::shared_ptr<Map> testMap()
{
    std::shared_ptr<Map> map = std::make_shared<Map>();

    // for (int x = 5; x < 20; ++x)
    // {
    //     for (int y = 5; y < 20; ++y)
    //     {
    //         Item gold(2148);
    //         Item something(2152);
    //         gold.setCount(25);
    //         // map->addItem(Position(x, y, 7), 4526);
    //         map->addItem(Position(x, y, 7), std::move(gold));
    //         map->addItem(Position(x, y, 7), std::move(something));
    //     }
    // }

    {
        Item gold(2148);
        Item platinum(2152);
        gold.setCount(25);
        map->addItem(Position(7, 7, 7), 4526);
        map->addItem(Position(7, 7, 7), std::move(gold));
        map->addItem(Position(7, 7, 7), std::move(platinum));
    }
    {
        Item gold(2148);
        gold.setCount(25);
        map->addItem(Position(8, 8, 7), 4526);
        map->addItem(Position(8, 8, 7), std::move(gold));
    }
    {
        map->addItem(Position(9, 9, 7), 4526);
    }

    return map;
}

MapViewItemRenderer::MapViewItemRenderer()
    : action(),
      mapView(std::make_unique<MockUIUtils>(), action, testMap()),
      vulkanInfo(),
      renderer(vulkanInfo, &mapView),
      screenTexture()
{
    mapView.setX(0);
    mapView.setY(0);
    mapView.setViewportSize(1920, 1080);
}

MapViewItemRenderer::~MapViewItemRenderer()
{
}

void MapViewItem::sync()
{
    if (!m_renderer)
    {
        m_renderer = new MapViewItemRenderer;
        // Initializing resources is done before starting to record the
        // renderpass, regardless of wanting an underlay or overlay.
        connect(window(), &QQuickWindow::beforeRendering, m_renderer, &MapViewItemRenderer::frameStart, Qt::DirectConnection);
        // Here we want an underlay and therefore connect to
        // beforeRenderPassRecording. Changing to afterRenderPassRecording
        // would render on top (overlay).
        connect(window(), &QQuickWindow::beforeRenderPassRecording, m_renderer, &MapViewItemRenderer::mainPassRecordingStart, Qt::DirectConnection);
    }
    m_renderer->setViewportSize(window()->size() * window()->devicePixelRatio());
    m_renderer->setWindow(window());
}

void MapViewItemRenderer::frameStart()
{
    const QQuickWindow::GraphicsStateInfo &stateInfo(m_window->graphicsStateInfo());
    QSGRendererInterface *rif = m_window->rendererInterface();

    // We are not prepared for anything other than running with the RHI and its Vulkan backend.
    Q_ASSERT(rif->graphicsApi() == QSGRendererInterface::Vulkan);

    if (!m_initialized)
    {
        vulkanInfo.setMaxConcurrentFrameCount(stateInfo.framesInFlight);

        VkSurfaceKHR surface = m_window->vulkanInstance()->surfaceForWindow(m_window);

        QSize size = m_window->size() * m_window->devicePixelRatio();
        uint32_t width = static_cast<uint32_t>(size.width());
        uint32_t height = static_cast<uint32_t>(size.height());

        renderer.initResources(surface, width, height);
        m_initialized = true;
    }
}

void MapViewItemRenderer::mainPassRecordingStart()
{
    // This example demonstrates the simple case: prepending some commands to
    // the scenegraph's main renderpass. It does not create its own passes,
    // rendertargets, etc. so no synchronization is needed.

    const QQuickWindow::GraphicsStateInfo &stateInfo(m_window->graphicsStateInfo());
    QSGRendererInterface *rif = m_window->rendererInterface();

    m_window->beginExternalCommands();

    // Must query the command buffer _after_ beginExternalCommands(), this is
    // actually important when running on Vulkan because what we get here is a
    // new secondary command buffer, not the primary one.
    VkCommandBuffer cb = *reinterpret_cast<VkCommandBuffer *>(
        rif->getResource(m_window, QSGRendererInterface::CommandListResource));
    Q_ASSERT(cb);

    VME_LOG_D("In flight: " << stateInfo.framesInFlight << ", current: " << stateInfo.currentFrameSlot);

    renderer.setCurrentFrame(stateInfo.currentFrameSlot);
    auto frame = renderer.currentFrame();
    frame->currentFrameIndex = stateInfo.currentFrameSlot;
    frame->commandBuffer = cb;

    // TODO Set framebuffer here (or in map_renderer.cpp) based on swap chain!
    frame->frameBuffer = VK_NULL_HANDLE;

    // TODO
    // frame->mouseAction = window.mapView->editorAction.action();

    renderer.render(screenTexture.vkFrameBuffer());
    // if (renderer._containsAnimation && Settings::RENDER_ANIMATIONS)
    // {
    //     QTimer::singleShot(50, [this]() { window.requestUpdate(); });
    // }

    // Do not assume any state persists on the command buffer. (it may be a
    // brand new one that just started recording)

    m_window->endExternalCommands();
}

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

// UI Utils

ScreenPosition MockUIUtils::mouseScreenPosInView()
{
    return ScreenPosition(0, 0);
}

double MockUIUtils::screenDevicePixelRatio()
{
    return 1.0;
}

double MockUIUtils::windowDevicePixelRatio()
{
    return 1.0;
}

VME::ModifierKeys MockUIUtils::modifiers() const
{
    return VME::ModifierKeys::None;
}

void MockUIUtils::waitForDraw(std::function<void()> f)
{
    // No-op
}