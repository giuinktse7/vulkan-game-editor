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

#include "items.h"
#include "time_point.h"

QLabel *itemImage(uint16_t serverId)
{
    ItemType *t = Items::items.getItemType(serverId);
    const TextureInfo &info = t->getTextureInfoUnNormalized();

    const uint8_t *pixelData = info.atlas->getOrCreateTexture().pixels().data();
    QImage img(pixelData, 12 * 32, 12 * 32, 384 * 4, QImage::Format::Format_ARGB32);

    QPixmap pixelMap = QPixmap::fromImage(img);

    QRect textureRegion(info.window.x0, info.window.y0, info.window.x1, info.window.y1);

    QLabel *container = new QLabel;
    container->setPixmap(pixelMap.copy(textureRegion).transformed(QTransform().scale(1, -1)));

    return container;
}

MainWindow::MainWindow(VulkanWindow *vulkanWindow)
{
    QWidget *wrapper = vulkanWindow->wrapInWidget();
    // wrapper->setFocusPolicy(Qt::StrongFocus);
    // wrapper->setFocus();

    setWindowTitle("Vulkan editor");

    rootLayout = new QVBoxLayout;
    createMenuBar();

    QVBoxLayout *testLayout = new QVBoxLayout;

    textEdit = new QPlainTextEdit;
    textEdit->setReadOnly(false);
    textEdit->setPlainText("100");
    textEdit->setMaximumHeight(80);
    testLayout->addWidget(textEdit);

    // for (uint16_t id = 100; id < Items::items.size(); ++id)
    // {
    //     ItemType *t = Items::items.getItemType(id);

    //     if (!Items::items.validItemType(id))
    //         continue;
    //     itemImage(id);
    // }

    // testLayout->addWidget(itemImage(100));
    // testLayout->addWidget(itemImage(2554));
    // testLayout->addWidget(itemImage(2148));

    QGridLayout *gridLayout = new QGridLayout;

    gridLayout->addLayout(testLayout, 0, 0);

    mapTabs = new QTabWidget(this);
    mapTabs->setTabsClosable(true);
    QObject::connect(mapTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeMapTab(int)));

    mapTabs->addTab(wrapper, "untitled.otbm");

    gridLayout->addWidget(mapTabs, 1, 0, 7, 2);

    rootLayout->addItem(gridLayout);

    setLayout(rootLayout);
}

void MainWindow::mousePressEvent(QMouseEvent *)
{
    VME_LOG_D("MainWindow::mousePressEvent");
}

void MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar;
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));

    QAction *_new = new QAction(tr("&New"), this);
    fileMenu->addAction(_new);

    QMenu *editMenu = menuBar->addMenu(tr("&Edit"));

    QAction *undo = new QAction(tr("&Undo"), this);
    editMenu->addAction(undo);

    QAction *redo = new QAction(tr("&Redo"), this);
    editMenu->addAction(redo);

    this->rootLayout->setMenuBar(menuBar);
}

void MainWindow::closeMapTab(int index)
{
    std::cout << "MainWindow::closeMapTab" << std::endl;
    this->mapTabs->widget(index)->deleteLater();
    this->mapTabs->removeTab(index);
}