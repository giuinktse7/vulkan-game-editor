
#include <iostream>
#include <QApplication>
#include <QVulkanInstance>
#include <QLoggingCategory>

#include <memory>
#include <optional>

#include "gui/mainwindow.h"
#include "gui/vulkan_window.h"

#include "graphics/vulkan_helpers.h"
#include "graphics/appearances.h"
#include "items.h"

void loadGameData()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    g_ecs.registerComponent<ItemAnimationComponent>();
    g_ecs.registerSystem<ItemAnimationSystem>();

    Appearances::loadTextureAtlases("data/catalog-content.json");
    Appearances::loadAppearanceData("data/appearances.dat");

    Items::loadFromOtb("data/items.otb");
    Items::loadFromXml("data/items.xml");
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const bool dbg = qEnvironmentVariableIntValue("QT_VK_DEBUG");

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    loadGameData();

    QVulkanInstance instance;

    instance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    if (!instance.create())
        qFatal("Failed to create Vulkan instance: %d", instance.errorCode());

    auto mapView = std::make_unique<MapView>();

    mapView->history.startGroup(ActionGroupType::AddMapItem);
    mapView->addItem(Position(5, 6, 7), 2148);
    mapView->addItem(Position(5, 5, 7), 2706);
    for (int x = 6; x < 10; ++x)
    {
        for (int y = 6; y < 10; ++y)
        {
            mapView->addItem(Position(x, y, 7), 100);
        }
    }
    mapView->history.endGroup(ActionGroupType::AddMapItem);

    std::unique_ptr<VulkanWindow> vulkanWindow = std::make_unique<VulkanWindow>(std::move(mapView));
    VulkanWindow *windowPtr = vulkanWindow.get();

    vulkanWindow->setVulkanInstance(&instance);

    MainWindow mainWindow(windowPtr);
    mainWindow.resize(1024, 768);
    mainWindow.show();

    return app.exec();
}
