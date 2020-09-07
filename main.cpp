#include "main.h"

#include <iostream>
#include <memory>
#include <optional>

#include <QLoggingCategory>

#include "gui/mainwindow.h"
#include "gui/vulkan_window.h"

#include "graphics/vulkan_helpers.h"
#include "graphics/appearances.h"

#include "items.h"
#include "time_point.h"
#include "random.h"

#define QT_MANAGED_POINTER(cls, ...) new cls(__VA_ARGS__);

void loadTextures()
{
    TimePoint start;
    for (uint16_t id = 100; id < Items::items.size(); ++id)
    {
        ItemType *t = Items::items.getItemType(id);

        if (!Items::items.validItemType(id))
            continue;

        const TextureInfo &info = t->getTextureInfoUnNormalized();
        info.atlas->getOrCreateTexture();
    }
}

void MainApplication::loadGameData()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    g_ecs.registerComponent<ItemAnimationComponent>();
    g_ecs.registerSystem<ItemAnimationSystem>();

    Appearances::loadTextureAtlases("data/catalog-content.json");
    Appearances::loadAppearanceData("data/appearances.dat");

    Items::loadFromOtb("data/items.otb");
    Items::loadFromXml("data/items.xml");

    // loadTextures();
}

MainApplication::MainApplication(int &argc, char **argv) : QApplication(argc, argv)
{
    connect(this, &QApplication::applicationStateChanged, this, &MainApplication::onApplicationStateChanged);
    connect(this, &QApplication::focusWindowChanged, this, &MainApplication::onFocusWindowChanged);
    connect(this, &QApplication::focusChanged, this, &MainApplication::onFocusWidgetChanged);
}

void MainApplication::setVulkanWindow(VulkanWindow *window)
{
    this->vulkanWindow = window;
}

void MainApplication::onApplicationStateChanged(Qt::ApplicationState state)
{
    if (state == Qt::ApplicationState::ApplicationActive)
    {
        if (focusedWindow == vulkanWindow)
        {
            if (!vulkanWindow->isActive())
            {
                vulkanWindow->requestActivate();
            }
        }
        else
        {
            bool hasFocusedWidget = focusedWindow != nullptr && focusedWindow->focusObject() == nullptr;
            if (hasFocusedWidget)
            {
                if (!focusedWindow->isActive())
                {
                    focusedWindow->requestActivate();
                }
            }
        }
    }
}

void MainApplication::onFocusWindowChanged(QWindow *window)
{
    if (window != nullptr)
    {
        focusedWindow = window;
    }
}

void MainApplication::onFocusWidgetChanged(QWidget *widget)
{
    prevWidget = currentWidget;
    currentWidget = widget;
}

int main(int argc, char *argv[])
{
    // return 0;
    Random::global().setSeed(123);
    TimePoint::setApplicationStartTimePoint();

    MainApplication app(argc, argv);
    app.loadGameData();

    // const bool dbg = qEnvironmentVariableIntValue("QT_VK_DEBUG");

    // QLoggingCategory::setFilterRules(QStringLiteral("'vulkan-map-editor.exe' (Win32)=false"));

    QVulkanInstance instance;

    instance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    if (!instance.create())
        qFatal("Failed to create Vulkan instance: %d", instance.errorCode());

    auto mapView = QT_MANAGED_POINTER(MapView);

    mapView->history.startGroup(ActionGroupType::AddMapItem);
    mapView->addItem(Position(4, 4, 7), 2706);
    mapView->addItem(Position(8, 10, 7), 2708);
    mapView->addItem(Position(2, 2, 7), 2554);
    // for (int x = 1; x < 6; ++x)
    // {
    //     for (int y = 1; y < 6; ++y)
    //     {
    //         mapView->addItem(Position(x, y, 7), 100);
    //     }
    // }
    mapView->history.endGroup(ActionGroupType::AddMapItem);

    VulkanWindow *vulkanWindow = QT_MANAGED_POINTER(VulkanWindow, mapView);
    app.setVulkanWindow(vulkanWindow);

    vulkanWindow->setVulkanInstance(&instance);

    MainWindow mainWindow(vulkanWindow);
    mainWindow.resize(1024, 768);
    mainWindow.show();
    vulkanWindow->requestActivate();
    // vulkanWindow->setAsDefaultWindow(app);

    return app.exec();
}
