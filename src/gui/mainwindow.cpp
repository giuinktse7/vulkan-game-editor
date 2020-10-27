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
#include "../qt/logging.h"

#include "../main.h"

bool ItemListEventFilter::eventFilter(QObject *object, QEvent *event)
{
  switch (event->type())
  {
  case QEvent::KeyPress:
  {
    // VME_LOG_D("Focused widget: " << QApplication::focusWidget());
    auto keyEvent = static_cast<QKeyEvent *>(event);
    auto key = keyEvent->key();
    switch (key)
    {
    case Qt::Key_I:
    case Qt::Key_Space:
    {
      auto widget = QtUtil::qtApp()->widgetAt(QCursor::pos());
      auto vulkanWindow = QtUtil::associatedVulkanWindow(widget);
      if (vulkanWindow)
      {
        QApplication::sendEvent(vulkanWindow, event);
      }

      return false;
      break;
    }
    }
    break;
  }
  default:
    break;
  }

  return QObject::eventFilter(object, event);
}

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
  VulkanWindow *vulkanWindow = QT_MANAGED_POINTER(VulkanWindow, map, editorAction);

  // Window setup
  vulkanWindow->setVulkanInstance(vulkanInstance);
  vulkanWindow->debugName = map->name().empty() ? toString(vulkanWindow) : map->name();

  MouseAction::RawItem action;
  action.serverId = 6217;
  editorAction.set(action);

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
  listView->installEventFilter(new ItemListEventFilter(this));
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
    editorAction.set(action);
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
  case Qt::Key_Delete:
    mapTabs->currentMapView()->deleteSelection();
    break;
  default:
  {
    auto widget = QtUtil::qtApp()->widgetAt(QCursor::pos());
    auto vulkanWindow = QtUtil::associatedVulkanWindow(widget);
    if (vulkanWindow)
    {
      QApplication::sendEvent(vulkanWindow, event);
    }
  }
  // event->ignore();
  // QWidget::keyPressEvent(event);

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

  {
    QAction *debug = new QAction(tr("Toggle debug"), this);
    connect(debug, &QAction::triggered, [=] { DEBUG_FLAG_ACTIVE = !DEBUG_FLAG_ACTIVE; });
    menuBar->addAction(debug);
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
