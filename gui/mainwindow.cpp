#include "mainwindow.h"

#include <QGridLayout>
#include <QPlainTextEdit>
#include <QMenu>
#include <QTabWidget>
#include <QMenuBar>
#include <QtWidgets>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QListView>

#include "vulkan_window.h"

#include "items.h"
#include "time_point.h"
#include "item_list.h"
#include "qt_util.h"
#include "border_layout.h"

QLabel *itemImage(uint16_t serverId)
{
    QLabel *container = new QLabel;
    container->setPixmap(QtUtil::itemPixmap(serverId));

    return container;
}

void MainWindow::experimentLayout()
{
    // rootLayout = new QVBoxLayout;
    // setLayout(rootLayout);

    // std::vector<QString> data;
    // for (int i = 0; i < 50; ++i)
    // {
    //     data.push_back("" + QString::number(i));
    // }

    // QListView *listView = new QListView();
    // QtItemTypeModel *model = new QtItemTypeModel(listView);
    // model->populate(std::move(data));

    // listView->setModel(model);
    // listView->setRootIndex(model->index(0, 0));
    // listView->setCurrentIndex(listView->rootIndex());

    // QPushButton *button = new QPushButton("&Test", this);
    // button->connect(button, &QPushButton::clicked, [=](bool checked) {
    //     VME_LOG_D("Current data: " << listView->currentIndex().data(Qt::DisplayRole).toString().toStdString());
    //     auto model = listView->currentIndex();
    //     int x = 2;
    //     // VME_LOG_D(model->_data.at(0).toStdString());
    // });

    // rootLayout->addWidget(listView);
    // rootLayout->addWidget(button);
}

void MainWindow::addMapTab(VulkanWindow &vulkanWindow)
{
    QWidget *wrapper = vulkanWindow.wrapInWidget();

    wrapper->setFocusPolicy(Qt::StrongFocus);
    wrapper->setFocus();

    mapTabs->addTab(wrapper, "untitled.otbm");
}

void MainWindow::experiment2()
{
    createMapTabArea();

    BorderLayout *borderLayout = new BorderLayout;
    rootLayout = borderLayout;

    QMenuBar *menu = createMenuBar();
    rootLayout->setMenuBar(menu);

    QListView *listView = new QListView;
    listView->setItemDelegate(new Delegate(this));

    std::vector<ItemTypeModelItem> data;
    data.push_back(ItemTypeModelItem::fromServerId(2554));
    data.push_back(ItemTypeModelItem::fromServerId(2148));
    data.push_back(ItemTypeModelItem::fromServerId(2555));

    QtItemTypeModel *model = new QtItemTypeModel(listView);
    model->populate(std::move(data));

    listView->setModel(model);
    borderLayout->addWidget(listView, BorderLayout::Position::West);

    textEdit = new QPlainTextEdit;
    textEdit->setReadOnly(false);
    textEdit->setPlainText("100");
    textEdit->setMaximumHeight(80);

    borderLayout->addWidget(mapTabs, BorderLayout::Position::Center);

    setLayout(rootLayout);
}

MainWindow::MainWindow()
{
    setWindowTitle("Vulkan editor");

    experiment2();
}

void MainWindow::createMapTabArea()
{
    mapTabs = new QTabWidget(this);
    mapTabs->setTabsClosable(true);
    QObject::connect(mapTabs, SIGNAL(tabCloseRequested(int)), this, SLOT(closeMapTab(int)));
}

void MainWindow::mousePressEvent(QMouseEvent *)
{
    VME_LOG_D("MainWindow::mousePressEvent");
}

QMenuBar *MainWindow::createMenuBar()
{

    QMenuBar *menuBar = new QMenuBar;
    menuBar->setStyleSheet("QMenu { padding: red }");
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));

    QAction *_new = new QAction(tr("&New"), this);
    fileMenu->addAction(_new);

    QMenu *editMenu = menuBar->addMenu(tr("&Edit"));

    QAction *undo = new QAction(tr("&Undo"), this);
    editMenu->addAction(undo);

    QAction *redo = new QAction(tr("&Redo"), this);
    editMenu->addAction(redo);

    return menuBar;
}

void MainWindow::closeMapTab(int index)
{
    std::cout << "MainWindow::closeMapTab" << std::endl;
    this->mapTabs->widget(index)->deleteLater();
    this->mapTabs->removeTab(index);
}