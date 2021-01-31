#include "main_application.h"

#include <QFile>

MainApplication::MainApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
    connect(this, &QApplication::applicationStateChanged, this, &MainApplication::onApplicationStateChanged);
    connect(this, &QApplication::focusWindowChanged, this, &MainApplication::onFocusWindowChanged);
    connect(this, &QApplication::focusChanged, this, &MainApplication::onFocusWidgetChanged);

    QApplication::setEffectEnabled(Qt::UI_AnimateCombo, false);

    loadStyleSheet(":/vme/style/qss/default.qss");

    vulkanInstance.setLayers(QByteArrayList() << "VK_LAYER_LUNARG_standard_validation");

    if (!vulkanInstance.create())
        qFatal("Failed to create Vulkan instance: %d", vulkanInstance.errorCode());

    mainWindow.setVulkanInstance(&vulkanInstance);
    mainWindow.resize(1024, 768);
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

int MainApplication::run()
{
    mainWindow.show();
    return exec();
}

void MainApplication::initializeUI()
{
    mainWindow.initializeUI();

    // mainWindow.editorAction.setRawItem(2148);
    mainWindow.editorAction.setRawBrush(5050);
}

void MainApplication::loadStyleSheet(const QString &path)
{
    QFile file(path);
    file.open(QFile::ReadOnly);
    QString styleSheet = QString::fromLatin1(file.readAll());

    setStyleSheet(styleSheet);
}