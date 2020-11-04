#pragma once

#include <QApplication>
#include <QVulkanInstance>
#include <stdarg.h>
#include <string>

#include "gui/vulkan_window.h"

class MainApplication : public QApplication
{
public:
    MainApplication(int &argc, char **argv);

    void setVulkanWindow(VulkanWindow *window);
    void loadGameData();

public slots:
    void onApplicationStateChanged(Qt::ApplicationState state);
    void onFocusWindowChanged(QWindow *window);
    void onFocusWidgetChanged(QWidget *widget);

    void loadStyleSheet(const QString &path);

private:
    QWindow *focusedWindow = nullptr;
    QWidget *prevWidget = nullptr;
    QWidget *currentWidget = nullptr;

    QWindow *vulkanWindow = nullptr;
};