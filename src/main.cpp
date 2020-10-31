#include "main.h"

#include <iostream>
#include <memory>
#include <optional>

#include <QLoggingCategory>
#include <QFile>
#include <QHBoxLayout>
#include <QStyleFactory>

#include <QFontDatabase>
#include <QLabel>

#include "gui/borderless_window.h"
#include "gui/map_view_widget.h"

#include "graphics/appearances.h"

#include "items.h"
#include "time_point.h"
#include "random.h"
#include "util.h"

#include "qt/logging.h"

#include "ecs/ecs.h"

#include "gui/map_tab_widget.h"

/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/

#include "octree.h"

void addChunk(Position from, vme::octree::Tree &tree)
{
    auto chunk = vme::octree::ChunkSize;
    auto to = Position(from.x + chunk.width - 1, from.y + chunk.height - 1, from.z + chunk.depth - 1);
    VME_LOG("addChunk: " << from << " to " << to);

    for (const auto pos : MapArea(from, to))
        tree.add(pos);
}

void testOctree()
{
    VME_LOG_D("octree:");
    constexpr vme::MapSize mapSize = {2048, 2048, 16};
    // constexpr vme::MapSize mapSize = {4096, 4096, 16};

    vme::octree::Tree tree = vme::octree::Tree::create(mapSize);

    auto &rand = Random::global();
    rand.setSeed(123);

    // Positions that were bugged at one point(could include in tests maybe) : tree.add(Position(712, 428, 10));
    // tree.add(Position(718, 491, 12));
    // tree.add(Position(67, 473, 8));
    // tree.add(Position(47, 89, 7));

    // tree.add(Position(960, 2752, 8));
    // tree.add(Position(3001, 2773, 8));

    tree.add(Position(35, 35, 7));
    // tree.remove(Position(35, 35, 7));
    tree.add(Position(64, 32, 7));

    // auto chunk = vme::octree::ChunkSize;
    // int chunksX = 1;
    // int chunksY = 1;
    // int chunksZ = 2;
    // int positions = chunksX * chunksY * chunksZ * (chunk.width * chunk.height * chunk.depth);

    // VME_LOG("Adding from " << Position(0, 0, 0) << " to " << Position(chunksX * chunk.width, chunksY * chunk.height, chunksZ * chunk.depth) << " (" << positions << " positions).");
    // for (int x = 0; x < chunksX; ++x)
    //     for (int y = 0; y < chunksY; ++y)
    //         for (int z = 0; z < chunksZ; ++z)
    //             addChunk(Position(x * 64, y * 64, z * 8), tree);

    int count = 0;

    TimePoint start;
    // for (const auto pos : tree)
    //     ++count;

    {
        // These two together created a crash (they shared the same leaf node)
        // tree.add(Position(979, 2778, 9));
        // tree.add(Position(3001, 2773, 8));
    }

    // for (int i = 0; i < 10000; ++i)
    // {
    //     auto x = rand.nextInt<int>(0, 4000);
    //     auto y = rand.nextInt<int>(0, 4000);
    //     auto z = rand.nextInt<int>(0, 16);

    //     Position pos(x, y, z);
    //     // VME_LOG_D("(" << i << "): " << pos);
    //     // VME_LOG_D("(" << i << "): "
    //     //   << "Position(" << x << "," << y << "," << z << ")");
    //     // VME_LOG_D("Adding " << pos);
    //     tree.add(pos);
    // }

    // {

    // auto b = Position(3638, 873, 5);
    // tree.add(b);

    // auto a = Position(3644, 797, 0);
    // tree.add(a);

    // VME_LOG_D(tree.boundingBox());

    // tree.remove(b);

    // VME_LOG_D(tree.boundingBox());

    // }

    // {
    //     auto a = Position(628, 1354, 7);
    //     tree.add(a);
    //     auto b = Position(2518, 1604, 11);
    //     tree.add(b);

    //     auto c = Position(2507, 1788, 14);
    //     tree.add(c);
    // }

    // auto pos = Position(47, 89, 7);
    // tree.add(pos);

    // VME_LOG_D("Next:");
    // auto pos2 = Position(1027, 287, 9);
    // tree.add(pos2);

    // VME_LOG_D("pos2: " << std::boolalpha << tree.contains(pos2));
    // tree.remove(pos2);
    // VME_LOG_D("pos2 still here? " << std::boolalpha << tree.contains(pos2));
    VME_LOG_D("Count: " << count);
    VME_LOG("Done in " << start.elapsedMicros() << " us. (" << start.elapsedMillis() << " ms)");

    TimePoint clearStart;
    tree.clear();

    VME_LOG("Clear in " << clearStart.elapsedMicros() << " us. Size: " << tree.size() << ", contains: " << tree.contains(Position(10, 10, 7)));
}
/*********************************************************/
/*********************************************************/
/*********************************************************/
/*********************************************************/

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

std::shared_ptr<Map> makeTestMap1()
{
    std::shared_ptr<Map> map = std::make_shared<Map>();
    auto &rand = Random::global();

    for (int x = 0; x < 200; ++x)
    {
        for (int y = 0; y < 200; ++y)
        {
            map->addItem(Position(x, y, 7), rand.nextInt<uint16_t>(4526, 4542));
        }
    }

    for (int i = 0; i < 10; ++i)
    {
        map->addItem(Position(rand.nextInt<uint16_t>(1, 10), rand.nextInt<uint16_t>(1, 10), 7), 2767 + rand.nextInt<uint16_t>(0, 2));
    }

    return map;
}

std::shared_ptr<Map> makeTestMap2()
{

    std::shared_ptr<Map> map = std::make_shared<Map>();
    auto &rand = Random::global();

    int i = 0;
    for (int y = 0; y < 10; ++y)
    {
        for (int x = 0; x < 10; ++x)
        {
            map->addItem(Position(1 + x, 1 + y, 7), 35950 + i);
            ++i;
        }
    }

    std::array<uint16_t, 9> sandWaterBorders{7951, 7945, 7952, 7946, 4608, 7944, 7953, 7943, 7954};

    Position pos(10, 10, 7);
    map->addItem(pos, sandWaterBorders[0]);

    pos.move(1, 0, 0);
    map->addItem(pos, sandWaterBorders[1]);

    pos.move(1, 0, 0);
    map->addItem(pos, sandWaterBorders[2]);

    pos.move(-2, 1, 0);
    map->addItem(pos, sandWaterBorders[3]);

    pos.move(1, 0, 0);
    map->addItem(pos, sandWaterBorders[4]);

    pos.move(1, 0, 0);
    map->addItem(pos, sandWaterBorders[5]);

    pos.move(-2, 1, 0);
    map->addItem(pos, sandWaterBorders[6]);

    pos.move(1, 0, 0);
    map->addItem(pos, sandWaterBorders[7]);

    pos.move(1, 0, 0);
    map->addItem(pos, sandWaterBorders[8]);

    return map;
}

int runApp(int argc, char *argv[])
{
    MainApplication app(argc, argv);
    app.loadStyleSheet("default");
    app.loadGameData();

    // ItemType *t = Items::items.getItemType(3817);
    // VME_LOG(t->getFirstTextureAtlas()->sourceFile);
    // return 0;

    MainWindow mainWindow;

    QVulkanInstance instance;

    instance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    if (!instance.create())
        qFatal("Failed to create Vulkan instance: %d", instance.errorCode());

    mainWindow.setVulkanInstance(&instance);

    {
        std::shared_ptr<Map> map = makeTestMap1();
        mainWindow.addMapTab(map);
    }

    {
        std::shared_ptr<Map> map = makeTestMap2();
        mainWindow.addMapTab(map);
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

    // testOctree();
    // return 0;

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
