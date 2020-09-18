#include "mainwindow.h"

#include <QWidget>
#include <QGridLayout>
#include <QPlainTextEdit>
#include <QMenu>
#include <QTabWidget>
#include <QMenuBar>
#include <QtWidgets>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QSlider>
#include <QListView>
#include <QSplitter>

#include "vulkan_window.h"
#include "item_list.h"
#include "qt_util.h"
#include "menu.h"
#include "border_layout.h"
#include "map_view_widget.h"
#include "map_tab_widget.h"

#include "../main.h"

QLabel *itemImage(uint16_t serverId)
{
  QLabel *container = new QLabel;
  container->setPixmap(QtUtil::itemPixmap(serverId));

  return container;
}

void MainWindow::addMapTab(VulkanWindow &vulkanWindow)
{
  QtMapViewWidget *widget = new QtMapViewWidget(&vulkanWindow);

  connect(&vulkanWindow, &VulkanWindow::mousePosEvent, [this, &vulkanWindow](util::Point<float> mousePos) {
    Position pos = vulkanWindow.getMapView()->toPosition(mousePos);

    std::ostringstream s;
    s << pos;

    QString text = QString::fromStdString(s.str());
    this->positionStatus->setText(text);
  });

  connect(widget, &QtMapViewWidget::viewportChangedEvent, [this, &vulkanWindow](const Viewport &viewport) {
    MapView *mapView = vulkanWindow.getMapView();
    Position pos = mapView->mousePos().toPos(*mapView);

    std::ostringstream s;
    s << pos;

    QString text = QString::fromStdString(s.str());
    this->positionStatus->setText(text);
  });

  mapTabs->addTabWithButton(widget, "untitled.otbm");
}

void MainWindow::initializeUI()
{
  mapTabs = new MapTabWidget(this);
  {
    auto l = new QLabel("Hey");

    mapTabs->addTabWithButton(l, "First");
  }

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
  rootLayout->addWidget(listView, BorderLayout::Position::West);

  rootLayout->addWidget(mapTabs, BorderLayout::Position::Center);

  QSlider *slider = new QSlider;
  slider->setMinimum(0);
  slider->setMaximum(15);
  slider->setSingleStep(1);
  slider->setPageStep(5);

  rootLayout->addWidget(slider, BorderLayout::Position::East);

  QWidget *bottomStatusBar = new QWidget;
  QHBoxLayout *bottomLayout = new QHBoxLayout;
  bottomStatusBar->setLayout(bottomLayout);

  positionStatus->setText("Test");
  bottomLayout->addWidget(positionStatus);

  rootLayout->addWidget(bottomStatusBar, BorderLayout::Position::South);

  setLayout(rootLayout);
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      rootLayout(new BorderLayout),
      positionStatus(new QLabel)
{
  initializeUI();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
  VME_LOG_D("MainWindow::mousePressEvent");
}

QMenuBar *MainWindow::createMenuBar()
{
  QMenuBar *menuBar = new QMenuBar;

  // File
  {
    auto fileMenu = menuBar->addMenu(tr("File"));

    auto newMap = new MenuAction(tr("New Map"), Qt::CTRL + Qt::Key_N, this);
    connect(newMap, &QWidgetAction::triggered, [=] { VME_LOG_D("New Map clicked"); });
    fileMenu->addAction(newMap);
  }

  // Edit
  {
    auto editMenu = menuBar->addMenu(tr("Edit"));

    auto undo = new MenuAction(tr("Undo"), Qt::CTRL + Qt::Key_Z, this);
    editMenu->addAction(undo);

    auto redo = new MenuAction(tr("Redo"), Qt::CTRL + Qt::SHIFT + Qt::Key_Z, this);
    editMenu->addAction(redo);

    editMenu->addSeparator();

    auto cut = new MenuAction(tr("Cut"), Qt::CTRL + Qt::Key_X, this);
    editMenu->addAction(cut);

    auto copy = new MenuAction(tr("Copy"), Qt::CTRL + Qt::Key_C, this);
    editMenu->addAction(copy);

    auto paste = new MenuAction(tr("Paste"), Qt::CTRL + Qt::Key_V, this);
    editMenu->addAction(paste);
  }

  // Map
  {
    auto mapMenu = menuBar->addMenu(tr("Map"));

    auto editTowns = new MenuAction(tr("Edit Towns"), Qt::CTRL + Qt::Key_T, this);
    mapMenu->addAction(editTowns);
  }

  // View
  {
    auto viewMenu = menuBar->addMenu(tr("View"));

    auto zoomIn = new MenuAction(tr("Zoom in"), Qt::CTRL + Qt::Key_Plus, this);
    viewMenu->addAction(zoomIn);

    auto zoomOut = new MenuAction(tr("Zoom out"), Qt::CTRL + Qt::Key_Minus, this);
    viewMenu->addAction(zoomOut);
  }

  // Window
  {
    auto windowMenu = menuBar->addMenu(tr("Window"));

    auto minimap = new MenuAction(tr("Minimap"), Qt::Key_M, this);
    windowMenu->addAction(minimap);
  }

  // Floor
  {
    auto floorMenu = menuBar->addMenu(tr("Floor"));

    QString floor = tr("Floor") + " ";
    for (int i = 0; i < 16; ++i)
    {
      auto floorI = new MenuAction(floor + QString::number(i), Qt::Key_M, this);
      floorMenu->addAction(floorI);
    }
  }

  {
    QAction *reloadStyles = new QAction(tr("Reload styles"), this);
    connect(reloadStyles, &QAction::triggered, [=] { QtUtil::qtApp()->loadStyleSheet("default"); });
    menuBar->addAction(reloadStyles);
  }

  return menuBar;
}
void MainWindow::setVulkanInstance(QVulkanInstance *instance)
{
  vulkanInstance = instance;
}
