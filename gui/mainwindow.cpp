#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGridLayout>

#include "vulkan_window.h"

MainWindow::MainWindow(VulkanWindow *vulkanWindow)
{
    QWidget *wrapper = QWidget::createWindowContainer(vulkanWindow);
    wrapper->setFocusPolicy(Qt::StrongFocus);
    wrapper->setFocus();

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(wrapper, 0, 0, 7, 2);
    setLayout(layout);
}
