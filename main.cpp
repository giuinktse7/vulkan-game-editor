
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

int main(int argc, char *argv[])
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    QApplication app(argc, argv);

    const bool dbg = qEnvironmentVariableIntValue("QT_VK_DEBUG");

    if (dbg)
    {
        QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
    }

    qDebug() << "Starting debugging..";

    g_ecs.registerComponent<ItemAnimationComponent>();
    g_ecs.registerSystem<ItemAnimationSystem>();

    Appearances::loadTextureAtlases("data/catalog-content.json");
    Appearances::loadAppearanceData("data/appearances.dat");

    Items::loadFromOtb("data/items.otb");
    Items::loadFromXml("data/items.xml");

    QVulkanInstance instance;
    // instance.setFlags(QVulkanInstance::NoDebugOutputRedirect);

    instance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    if (!instance.create())
        qFatal("Failed to create Vulkan instance: %d", instance.errorCode());

    auto mapView = std::make_unique<MapView>();

    mapView->history.startGroup(ActionGroupType::AddMapItem);
    mapView->addItem(Position(5, 5, 7), 2148);
    mapView->addItem(Position(5, 5, 7), 100);
    mapView->addItem(Position(5, 5, 7), 2706);
    mapView->history.endGroup(ActionGroupType::AddMapItem);

    std::unique_ptr<VulkanWindow> vulkanWindow = std::make_unique<VulkanWindow>(std::move(mapView));

    vulkanWindow->setVulkanInstance(&instance);

    MainWindow window(vulkanWindow.get());
    window.resize(1024, 768);
    window.show();

    return app.exec();
}
