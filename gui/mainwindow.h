#pragma once

#include <QWidget>

class VulkanWindow;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(VulkanWindow *vulkanWindow);

private:
};
