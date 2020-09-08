#pragma once

#include <QApplication>
#include <QVulkanInstance>
#include <stdarg.h>

#include "gui/mainwindow.h"
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

private:
    QWindow *focusedWindow = nullptr;
    QWidget *prevWidget = nullptr;
    QWidget *currentWidget = nullptr;

    QWindow *vulkanWindow = nullptr;
};