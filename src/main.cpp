#include "main.h"

#include <iostream>
#include <memory>
#include <optional>

#include <QLoggingCategory>
#include <QFile>
#include <QHBoxLayout>

#include <QFontDatabase>

#include "gui/borderless_window.h"
#include "gui/map_view_widget.h"

#include "graphics/vulkan_helpers.h"
#include "graphics/appearances.h"

#include "items.h"
#include "time_point.h"
#include "random.h"

#include "qt/logging.h"

#include "ecs/ecs.h"

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
    VME_LOG_D("loadTextures() ms: " << start.elapsedMillis());
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
    // if (state == Qt::ApplicationState::ApplicationActive)
    // {
    //     if (focusedWindow == vulkanWindow)
    //     {
    //         if (!vulkanWindow->isActive())
    //         {
    //             vulkanWindow->requestActivate();
    //         }
    //     }
    //     else
    //     {
    //         bool hasFocusedWidget = focusedWindow != nullptr && focusedWindow->focusObject() == nullptr;
    //         if (hasFocusedWidget)
    //         {
    //             if (!focusedWindow->isActive())
    //             {
    //                 focusedWindow->requestActivate();
    //             }
    //         }
    //     }
    // }
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

void makeTestMap1(MapView *mapView)
{
    auto &rand = Random::global();

    mapView->history.startGroup(ActionGroupType::AddMapItem);

    int i = 0;
    for (int x = 0; x < 30; ++x)
    {
        for (int y = 0; y < 30; ++y)
        {
            mapView->addItem(Position(1 + x, 1 + y, 7), 4526 + i);
            ++i;
        }
    }

    for (int i = 0; i < 10; ++i)
    {
        mapView->addItem(Position(rand.nextInt<uint16_t>(1, 10), rand.nextInt<uint16_t>(1, 10), 7), 2767 + rand.nextInt<uint16_t>(0, 2));
    }

    mapView->history.endGroup(ActionGroupType::AddMapItem);
}

void makeTestMap2(MapView *mapView)
{
    auto &rand = Random::global();

    mapView->history.startGroup(ActionGroupType::AddMapItem);

    for (int x = 0; x < 30; ++x)
    {
        for (int y = 0; y < 30; ++y)
        {
            mapView->addItem(Position(1 + x, 1 + y, 7), rand.nextInt<uint16_t>(4526, 4542));
        }
    }

    for (int i = 0; i < 10; ++i)
    {
        mapView->addItem(Position(rand.nextInt<uint16_t>(1, 10), rand.nextInt<uint16_t>(1, 10), 7), 2767 + rand.nextInt<uint16_t>(0, 2));
    }

    mapView->history.endGroup(ActionGroupType::AddMapItem);
}

int runApp(int argc, char *argv[])
{
    MainApplication app(argc, argv);
    app.loadStyleSheet("default");
    app.loadGameData();

    MainWindow mainWindow;

    QVulkanInstance instance;

    instance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    if (!instance.create())
        qFatal("Failed to create Vulkan instance: %d", instance.errorCode());

    mainWindow.setVulkanInstance(&instance);
    {
        VulkanWindow *vulkanWindow = QT_MANAGED_POINTER(VulkanWindow, std::make_unique<MapView>());
        vulkanWindow->setVulkanInstance(&instance);
        vulkanWindow->debugName = "Window1";

        MapView *mapView = vulkanWindow->getMapView();
        MapView::MouseAction::RawItem action;
        action.serverId = 6217;
        mapView->setMouseAction(action);

        makeTestMap1(mapView);

        app.setVulkanWindow(vulkanWindow);

        mainWindow.addMapTab(*vulkanWindow);
    }
    {
        VulkanWindow *vulkanWindow = QT_MANAGED_POINTER(VulkanWindow, std::make_unique<MapView>());
        vulkanWindow->setVulkanInstance(&instance);
        vulkanWindow->debugName = "Window2";

        MapView *mapView = vulkanWindow->getMapView();
        MapView::MouseAction::RawItem action;
        action.serverId = 6217;
        mapView->setMouseAction(action);

        makeTestMap2(mapView);

        app.setVulkanWindow(vulkanWindow);

        mainWindow.addMapTab(*vulkanWindow);
    }

    mainWindow.resize(1024, 768);
    mainWindow.show();

    QFontDatabase database;

    // const QStringList fontFamilies = database.families();
    // for (auto family : fontFamilies)
    // {
    //     VME_LOG_D(family);
    // }

    VME_LOG_D("???: " << database.hasFamily("Consolas"));

    return app.exec();
}

int main(int argc, char *argv[])
{
    Random::global().setSeed(123);
    TimePoint::setApplicationStartTimePoint();

    return runApp(argc, argv);
}

int borderlessTest(int argc, char *argv[])
{
    // MainApplication app(argc, argv);
    // // app.loadStyleSheet("default");
    // app.loadGameData();

    // // const bool dbg = qEnvironmentVariableIntValue("QT_VK_DEBUG");

    // // QLoggingCategory::setFilterRules(QStringLiteral("'vulkan-map-editor.exe' (Win32)=false"));

    // QVulkanInstance instance;

    // instance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    // if (!instance.create())
    //     qFatal("Failed to create Vulkan instance: %d", instance.errorCode());

    // auto mapView = std::make_shared<MapView>();

    // mapView->history.startGroup(ActionGroupType::AddMapItem);
    // mapView->addItem(Position(4, 4, 7), 2706);
    // mapView->addItem(Position(8, 10, 7), 2708);
    // mapView->addItem(Position(2, 2, 7), 2554);
    // mapView->history.endGroup(ActionGroupType::AddMapItem);

    // // VME_LOG_D("vulkanWindow: " << vulkanWindow);

    // BorderlessMainWindow *mainWindow = new BorderlessMainWindow(nullptr);
    // mainWindow->app = &app;

    // mainWindow->resize(1024, 768);
    // mainWindow->show();

    // VulkanWindow *vulkanWindow = QT_MANAGED_POINTER(VulkanWindow, mapView);
    // vulkanWindow->setParent(app.topLevelWindows().first());
    // app.setVulkanWindow(vulkanWindow);

    // vulkanWindow->setVulkanInstance(&instance);

    // mainWindow->addWidget(new QtMapViewWidget(vulkanWindow));

    // // vulkanWindow->requestActivate();

    // return app.exec();
    return -1;
}

void MainApplication::loadStyleSheet(const QString &sheetName)
{
    VME_LOG_D("loadStyleSheet");
    QFile file("resources/style/qss/" + sheetName.toLower() + ".qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QString::fromLatin1(file.readAll());

    setStyleSheet(styleSheet);
}
