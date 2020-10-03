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
#include <QVariant>
#include <QModelIndex>
#include <QVulkanInstance>

#include "vulkan_window.h"
#include "item_list.h"
#include "qt_util.h"
#include "menu.h"
#include "border_layout.h"
#include "map_view_widget.h"
#include "map_tab_widget.h"
#include "../util.h"
#include "qt_util.h"

#include "../main.h"

QLabel *itemImage(uint16_t serverId)
{
  QLabel *container = new QLabel;
  container->setPixmap(QtUtil::itemPixmap(serverId));

  return container;
}

uint32_t MainWindow::nextUntitledId()
{
  uint32_t id;
  if (untitledIds.empty())
  {
    ++highestUntitledId;

    id = highestUntitledId;
  }
  else
  {
    id = untitledIds.top();
    untitledIds.pop();
  }

  return id;
}

void MainWindow::addMapTab()
{
  addMapTab(std::make_shared<Map>());
}

void MainWindow::addMapTab(std::shared_ptr<Map> map)
{
  VulkanWindow *vulkanWindow = QT_MANAGED_POINTER(VulkanWindow, std::make_unique<MapView>(mapViewMouseAction, map), mapViewMouseAction);

  // Window setup
  vulkanWindow->setVulkanInstance(vulkanInstance);
  vulkanWindow->debugName = map->name().empty() ? toString(vulkanWindow) : map->name();

  MouseAction::RawItem action;
  action.serverId = 6217;
  mapViewMouseAction.set(action);

  // Create the widget
  MapViewWidget *widget = new MapViewWidget(vulkanWindow);

  connect(vulkanWindow, &VulkanWindow::mousePosChanged, [this, vulkanWindow](util::Point<float> mousePos) {
    mapViewMousePosEvent(*vulkanWindow->getMapView(), mousePos);
  });

  connect(widget, &MapViewWidget::viewportChangedEvent, [this, vulkanWindow](const Viewport &viewport) {
    mapViewViewportEvent(*vulkanWindow->getMapView(), viewport);
  });

  if (map->name().empty())
  {
    uint32_t untitledNameId = nextUntitledId();
    QString tabTitle = QString("Untitled-%1").arg(untitledNameId);

    mapTabs->addTabWithButton(widget, tabTitle, untitledNameId);
  }
  else
  {
    mapTabs->addTabWithButton(widget, QString::fromStdString(map->name()));
  }
}

void MainWindow::initializeUI()
{
  mapTabs = new MapTabWidget(this);
  connect(mapTabs, &MapTabWidget::mapTabClosed, [=](int index, QVariant data) {
    if (data.canConvert<uint32_t>())
    {
      uint32_t id = data.toInt();
      this->untitledIds.emplace(id);
    }
  });

  QMenuBar *menu = createMenuBar();
  rootLayout->setMenuBar(menu);

  QListView *listView = new QListView;
  listView->setItemDelegate(new Delegate(this));

  std::vector<ItemTypeModelItem> data;
  for (int i = 4500; i < 4700; ++i)
  {
    data.push_back(ItemTypeModelItem::fromServerId(i));
  }

  QtItemTypeModel *model = new QtItemTypeModel(listView);
  model->populate(std::move(data));

  listView->setModel(model);
  listView->setAlternatingRowColors(true);
  connect(listView, &QListView::clicked, [=](QModelIndex clickedIndex) {
    QVariant variant = listView->model()->data(clickedIndex);
    auto value = variant.value<ItemTypeModelItem>();

    MouseAction::RawItem action;
    action.serverId = value.itemType->id;
    mapViewMouseAction.set(action);
  });
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
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
  qDebug() << "MainWindow::keyPressEvent: " << event;
  // qDebug() << "MainWindow::keyPressEvent: " << event->type();
  switch (event->key())
  {
  case Qt::Key_Escape:
    mapTabs->currentMapView()->escapeEvent();
    break;
  case Qt::Key_0:
    if (event->modifiers() & Qt::CTRL)
    {
      mapTabs->currentMapView()->resetZoom();
    }
    break;
  case Qt::Key_I:
  {
    QWidget *widget = QtUtil::qtApp()->widgetAt(QCursor::pos());
    QVariant prop = widget->property("vulkan-window-wrapper");
    if (prop.canConvert<bool>() && prop.toBool())
    {
      VME_LOG_D("Yep: " << widget);
    }
    else
    {
      VME_LOG_D("Nop: " << widget);
    }

    // const Item *topItem = mapTabsmapView->map()->getTopItem(mapView->mouseGamePos());
    // if (topItem)
    // {
    // mapView->mapViewMouseAction.setRawItem(topItem->serverId());
    // }
    break;
  }
  case Qt::Key_Delete:
    mapTabs->currentMapView()->deleteSelectedItems();
    break;
  default:
    event->ignore();
    QWidget::keyPressEvent(event);
    break;
  }
}

QMenuBar *MainWindow::createMenuBar()
{
  QMenuBar *menuBar = new QMenuBar;

  // File
  {
    auto fileMenu = menuBar->addMenu(tr("File"));

    auto newMap = new MenuAction(tr("New Map"), Qt::CTRL + Qt::Key_N, this);
    connect(newMap, &QWidgetAction::triggered, [this] { this->addMapTab(); });
    fileMenu->addAction(newMap);

    auto closeMap = new MenuAction(tr("Close"), Qt::CTRL + Qt::Key_W, this);
    connect(closeMap, &QWidgetAction::triggered, mapTabs, &MapTabWidget::removeCurrentTab);
    fileMenu->addAction(closeMap);
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
      auto floorI = new MenuAction(floor + QString::number(i), this);
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

void MainWindow::mapViewMousePosEvent(MapView &mapView, util::Point<float> mousePos)
{
  Position pos = mapView.toPosition(mousePos);
  this->positionStatus->setText(toQString(pos));
}

void MainWindow::mapViewViewportEvent(MapView &mapView, const Viewport &viewport)
{
  Position pos = mapView.mousePos().toPos(mapView);
  this->positionStatus->setText(toQString(pos));
}

MapView *getMapViewOnCursor()
{
  QWidget *widget = QtUtil::qtApp()->widgetAt(QCursor::pos());
  return QtUtil::associatedMapView(widget);
}

bool MainWindow::globalKeyPressEvent(QKeyEvent *event)
{
  switch (event->key())
  {
  case Qt::Key_I:
  {

    /*// TODO!
      This will consume all clicks of the key 'i', even ones that are input into
      a text field. For this to work properly, true should only be returned here
      if the currently focused widget does not need the event.
    */
    MapView *mapView = getMapViewOnCursor();
    if (mapView)
    {
      const Item *topItem = mapView->map()->getTopItem(mapView->mouseGamePos());
      if (topItem)
      {
        mapView->mapViewMouseAction.setRawItem(topItem->serverId());
      }
    }
    return true;
  }
  break;
  default:
    break;
  }

  return false;
}

/*
 * This function receives ALL events in the application. This should mostly (if not only) be used
 * to hook events that should happen globally, regardless of focus.
 */
bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
  switch (event->type())
  {
  case QEvent::KeyPress:
  {
    if (globalKeyPressEvent(static_cast<QKeyEvent *>(event)))
    {
      return true;
    }
  }
  break;
  default:
    break;
  }

  return QWidget::eventFilter(object, event);
}
