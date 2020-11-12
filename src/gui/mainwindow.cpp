#include "mainwindow.h"

#include <QContextMenuEvent>
#include <QGridLayout>
#include <QListView>
#include <QMenu>
#include <QMenuBar>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QQuickView>
#include <QSlider>
#include <QTabWidget>
#include <QVariant>
#include <QVulkanInstance>
#include <QWidget>
#include <QtWidgets>

#include "../main.h"
#include "../map_io.h"
#include "../qt/logging.h"
#include "../util.h"
#include "border_layout.h"
#include "item_property_window.h"
#include "map_tab_widget.h"
#include "map_view_widget.h"
#include "menu.h"
#include "qt_util.h"
#include "split_widget.h"
#include "vulkan_window.h"

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

QLabel *itemImage(uint32_t serverId)
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

  // Create the widget
  MapViewWidget *widget = new MapViewWidget(vulkanWindow);

  connect(vulkanWindow,
          &VulkanWindow::mousePosChanged,
          [this, vulkanWindow](util::Point<float> mousePos) {
            mapViewMousePosEvent(*vulkanWindow->getMapView(), mousePos);
          });

  connect(widget,
          &MapViewWidget::viewportChangedEvent,
          [this, vulkanWindow](const Camera::Viewport &viewport) {
            mapViewViewportEvent(*vulkanWindow->getMapView(), viewport);
          });

  connect(widget,
          &MapViewWidget::selectionChangedEvent,
          [this, vulkanWindow]() {
            auto mapView = vulkanWindow->getMapView();
            if (mapView->singleTileSelected())
            {
              const Position pos = mapView->selection().onlyPosition().value();
              const Tile *tile = mapView->getTile(pos);
              DEBUG_ASSERT(tile != nullptr, "A tile that has a selection should never be nullptr.");

              if (tile->selectionCount() == 1)
              {
                auto item = tile->firstSelectedItem();
                DEBUG_ASSERT(item != nullptr, "It should be impossible for the selected item to be nullptr.");
                propertyWindow->setItem(*item);
              }
            }
          });

  if (map->name().empty())
  {
    uint32_t untitledNameId = nextUntitledId();
    QString mapName = QString("Untitled-%1.otbm").arg(untitledNameId);
    map->setName(mapName.toStdString());

    mapTabs->addTabWithButton(widget, mapName, untitledNameId);
  }
  else
  {
    mapTabs->addTabWithButton(widget, QString::fromStdString(map->name()));
  }
}

void MainWindow::mapTabCloseEvent(int index, QVariant data)
{
  if (data.canConvert<uint32_t>())
  {
    uint32_t id = data.toInt();
    this->untitledIds.emplace(id);
  }
}

void MainWindow::mapTabChangedEvent(int index)
{
  if (index == -1)
    return;

  // Empty for now
}

void MainWindow::initializeUI()
{
  mapTabs = new MapTabWidget(this);
  connect(mapTabs, &MapTabWidget::mapTabClosed, this, &MainWindow::mapTabCloseEvent);
  connect(mapTabs, &MapTabWidget::currentChanged, this, &MainWindow::mapTabChangedEvent);

  propertyWindow = new ItemPropertyWindow(QUrl("qrc:/vme/qml/itemPropertyWindow.qml"));
  connect(propertyWindow, &ItemPropertyWindow::countChanged, [this](int count) {
    VME_LOG_D("countChanged");
    MapView &mapView = *currentMapView();
    if (mapView.singleTileSelected())
    {
      const Position pos = mapView.selection().onlyPosition().value();
      const Tile *tile = mapView.getTile(pos);

      if (tile->firstSelectedItem()->count() != count)
      {
        mapView.update(TransactionType::ModifyItem, [&mapView, &pos, count] {
          mapView.modifyTile(pos, [count](Tile &tile) { tile.firstSelectedItem()->setCount(count); });
        });

        mapView.requestDraw();
      }
    }
  });

  QMenuBar *menu = createMenuBar();
  rootLayout->setMenuBar(menu);

  Splitter *splitter = new Splitter();
  rootLayout->addWidget(splitter, BorderLayout::Position::Center);

  auto itemPalette = createItemPalette();
  itemPalette->setMinimumWidth(240);
  itemPalette->setMaximumWidth(600);

  itemPalette->addItems(1533, 1542);
  itemPalette->addItems(5315, 5331);

  splitter->addWidget(itemPalette);
  splitter->setStretchFactor(0, 0);

  splitter->addWidget(mapTabs);
  splitter->setStretchFactor(1, 1);

  {
    auto container = propertyWindow->wrapInWidget();
    container->setMinimumWidth(200);
    splitter->addWidget(container);
    splitter->setStretchFactor(2, 0);
  }

  splitter->setSizes(QList<int>({200, 800, 200}));

  QWidget *bottomStatusBar = new QWidget;
  QHBoxLayout *bottomLayout = new QHBoxLayout;
  bottomStatusBar->setLayout(bottomLayout);

  positionStatus->setText("");
  bottomLayout->addWidget(positionStatus);

  zoomStatus->setText("");
  bottomLayout->addWidget(zoomStatus);

  creatureId->setText("");
  bottomLayout->addWidget(creatureId);

  rootLayout->addWidget(bottomStatusBar, BorderLayout::Position::South);

  setLayout(rootLayout);
}

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      rootLayout(new BorderLayout),
      positionStatus(new QLabel),
      zoomStatus(new QLabel),
      creatureId(new QLabel)
{

  // editorAction.onActionChanged<&MainWindow::editorActionChangedEvent>(this);
}

void MainWindow::editorActionChangedEvent(const MouseAction_t &action)
{
  // Currently unused
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
  switch (event->key())
  {
  case Qt::Key_Escape:
    currentMapView()->escapeEvent();
    break;
  case Qt::Key_0:
    if (event->modifiers() & Qt::CTRL)
    {
      currentMapView()->resetZoom();
    }
    break;
  case Qt::Key_Delete:
    currentMapView()->deleteSelectedItems();
    break;
  case Qt::Key_Z:
    if (event->modifiers() & Qt::CTRL)
    {
      currentMapView()->undo();
    }
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

ItemList *MainWindow::createItemPalette()
{
  ItemList *itemList = new ItemList;
  itemList->installEventFilter(new ItemListEventFilter(this));
  itemList->setItemDelegate(new Delegate(this));

  std::vector<ItemTypeModelItem> data;

  // addItemRange(103, 104);
  // addItemRange(1025, 1029);
  // addItemRange(20776, 20781);
  // addItemRange(21113, 21121);
  // addItemRange(25200, 25296);
  // addItemRange(33571, 36501);
  // addItemRange(23172, 23258);
  // addItemRange(33817, 33852);

  connect(itemList, &QListView::clicked, [this, itemList](QModelIndex index) {
    auto value = itemList->itemAtIndex(index);

    MouseAction::RawItem action;
    action.serverId = value.itemType->id;
    editorAction.setIfUnlocked(action);
  });

  return itemList;
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

    auto saveMap = new MenuAction(tr("Save"), Qt::CTRL + Qt::Key_S, this);
    connect(saveMap, &QWidgetAction::triggered, [this] { MapIO::saveMap(*(currentMapView()->map())); });
    fileMenu->addAction(saveMap);

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
    auto reloadMenu = menuBar->addMenu(tr("Reload"));

    QAction *reloadStyles = new QAction(tr("Reload styles"), this);
    connect(reloadStyles, &QAction::triggered, [=] { QtUtil::qtApp()->loadStyleSheet("default"); });
    reloadMenu->addAction(reloadStyles);

    QAction *reloadPropertyQml = new QAction(tr("Reload Properties QML"), this);
    connect(reloadPropertyQml, &QAction::triggered, [=] { propertyWindow->reloadSource(); });
    reloadMenu->addAction(reloadPropertyQml);
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
  auto tile = mapView.getTile(pos);
  if (tile && tile->hasCreature())
  {
    auto id = std::to_string(tile->creature()->creatureType.id());
    this->creatureId->setText("Creature ID: " + QString::fromStdString(id));
  }
  else
  {
    this->creatureId->setText("");
  }
  this->positionStatus->setText(toQString(pos));
}

void MainWindow::mapViewViewportEvent(MapView &mapView, const Camera::Viewport &viewport)
{
  Position pos = mapView.mousePos().toPos(mapView);
  this->positionStatus->setText(toQString(pos));
  this->zoomStatus->setText(toQString(std::round(mapView.getZoomFactor() * 100)) + "%");
}

MapView *MainWindow::currentMapView() const noexcept
{
  return mapTabs->currentMapView();
}

MapView *getMapViewOnCursor()
{
  QWidget *widget = QtUtil::qtApp()->widgetAt(QCursor::pos());
  return QtUtil::associatedMapView(widget);
}
