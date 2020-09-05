#include "mainwindow.h"

#include <QGridLayout>
#include <QPlainTextEdit>
#include <QMenu>
#include <QTabWidget>
#include <QMenuBar>
#include <QtWidgets>
#include <QContextMenuEvent>
#include <QMouseEvent>

#include "vulkan_window.h"

MainWindow::MainWindow(VulkanWindow *vulkanWindow)
{
    QWidget *wrapper = vulkanWindow->wrapInWidget();
    // wrapper->setFocusPolicy(Qt::StrongFocus);
    // wrapper->setFocus();

    setWindowTitle("Vulkan editor");

    rootLayout = new QVBoxLayout;
    createMenuBar();

    textEdit = new QPlainTextEdit;
    textEdit->setReadOnly(false);
    textEdit->setPlainText("100");
    textEdit->setMaximumHeight(80);

    QGridLayout *gridLayout = new QGridLayout;
    gridLayout->addWidget(textEdit, 0, 0);

    mapTabs = new QTabWidget(this);
    mapTabs->setTabsClosable(true);
    QObject::connect(mapTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeMapTab(int)));

    mapTabs->addTab(wrapper, "untitled.otbm");

    gridLayout->addWidget(mapTabs, 1, 0, 7, 2);

    rootLayout->addItem(gridLayout);

    setLayout(rootLayout);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    VME_LOG_D("MainWindow::mousePressEvent");
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar;
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));
    QMenu *editMenu = menuBar->addMenu(tr("&Edit"));

    QAction *newAction = new QAction(tr("&New"), this);
    fileMenu->addAction(newAction);

    this->rootLayout->setMenuBar(menuBar);
}

void MainWindow::closeMapTab(int index)
{
    std::cout << "MainWindow::closeMapTab" << std::endl;
    this->mapTabs->widget(index)->deleteLater();
    this->mapTabs->removeTab(index);
}