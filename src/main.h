#pragma once

#include <QApplication>
#include <QVulkanInstance>
#include <filesystem>
#include <optional>
#include <stdarg.h>
#include <string>
#include <utility>
#include <vector>

#include "gui/mainwindow.h"
#include "gui/vulkan_window.h"

namespace MainUtils
{
    void printOutfitAtlases(std::vector<uint32_t> outfitIds);
}

class MainApplication : public QApplication
{
public:
    MainApplication(int &argc, char **argv);

    std::pair<bool, std::optional<std::string>> loadGameData(std::string version);
    void initializeUI();

    int run();

    MainWindow mainWindow;

public slots:
    void onApplicationStateChanged(Qt::ApplicationState state);
    void onFocusWindowChanged(QWindow *window);
    void onFocusWidgetChanged(QWidget *widget);

    void loadStyleSheet(const QString &path);

private:
    QVulkanInstance vulkanInstance;

    QWindow *focusedWindow = nullptr;
    QWidget *prevWidget = nullptr;
    QWidget *currentWidget = nullptr;

    QWindow *vulkanWindow = nullptr;

    bool gameDataLoaded = false;
};