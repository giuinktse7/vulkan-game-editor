#pragma once

#include <QApplication>
#include <QVulkanInstance>
#include <stdarg.h>

class VulkanWindow;

class MainApplication : public QApplication
{
public:
    MainApplication(int &argc, char **argv);

    void setVulkanWindow(VulkanWindow *window);
    void loadGameData();
    QVulkanInstance createVulkanInstance();

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