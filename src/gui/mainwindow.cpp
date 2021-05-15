#include "mainwindow.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonportable-include-path"

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QQuickView>
#include <QSlider>
#include <QVariant>
#include <QVulkanInstance>
#include <QWidget>

#pragma clang diagnostic pop

#include "../brushes/ground_brush.h"
#include "../graphics/appearance_types.h"
#include "../item_location.h"
#include "../qt/logging.h"
#include "../save_map.h"
#include "../util.h"
#include "border_layout.h"
#include "itempalette/item_palette_model.h"
#include "itempalette/item_palette_window.h"
#include "main_application.h"
#include "map_tab_widget.h"
#include "map_view_widget.h"
#include "menu.h"
#include "properties/item_property_window.h"
#include "qt_util.h"
#include "split_widget.h"
#include "vulkan_window.h"

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
    vulkanWindow->mainWindow = this;

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
            this,
            &MainWindow::mapViewSelectionChangedEvent);

    connect(widget,
            &MapViewWidget::undoRedoEvent,
            this,
            &MainWindow::mapViewUndoRedoEvent);

    vulkanWindow->getMapView()->onSelectedItemClicked<&MainWindow::mapViewSelectedItemClicked>(this);

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

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent),
      _paletteWindow(nullptr),
      rootLayout(new BorderLayout),
      positionStatus(new QLabel),
      zoomStatus(new QLabel),
      topItemInfo(new QLabel)
{

    // editorAction.onActionChanged<&MainWindow::editorActionChangedEvent>(this);
}

//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>Events>>>>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>

void MainWindow::mapViewSelectedItemClicked(MapView *mapView, const Tile *tile, Item *item)
{
    auto position = tile->position();
    bool showInPropertyWindow = !(QApplication::keyboardModifiers() & Qt::KeyboardModifier::AltModifier);
    if (showInPropertyWindow)
    {
        propertyWindow->focusItem(item, position, *mapView);
    }
}

void MainWindow::mapViewSelectionChangedEvent(MapView &mapView)
{
    Item *selectedItem = mapView.singleSelectedItem();
    if (selectedItem)
    {
        auto position = mapView.singleSelectedTile()->position();
        bool showInPropertyWindow = !(QApplication::keyboardModifiers() & Qt::KeyboardModifier::AltModifier);
        if (showInPropertyWindow)
        {
            propertyWindow->focusItem(selectedItem, position, mapView);
        }
    }
    else
    {
        propertyWindow->resetFocus();
    }
}

void MainWindow::mapViewUndoRedoEvent(MapView &mapView)
{
    propertyWindow->refresh();
}

void MainWindow::mapViewMousePosEvent(MapView &mapView, util::Point<float> mousePos)
{
    Position pos = mapView.toPosition(mousePos);
    auto tile = mapView.getTile(pos);
    if (!tile || tile->isEmpty())
    {
        topItemInfo->setText("");
    }
    else if (tile->hasCreature())
    {
        auto id = std::to_string(tile->creature()->creatureType.id());
        topItemInfo->setText("Creature looktype: " + QString::fromStdString(id));
    }
    else if (tile->hasTopItem())
    {
        auto topItem = tile->getTopItem();
        std::ostringstream s;
        s << "Item \"" << topItem->name() << "\" id: " << topItem->serverId() << " cid: " << topItem->clientId();
        topItemInfo->setText(QString::fromStdString(s.str()));
    }

    positionStatus->setText(toQString(pos));
}

void MainWindow::mapViewViewportEvent(MapView &mapView, const Camera::Viewport &viewport)
{
    Position pos = mapView.mousePos().toPos(mapView);
    this->positionStatus->setText(toQString(pos));
    this->zoomStatus->setText(toQString(std::round(mapView.getZoomFactor() * 100)) + "%");
}

bool MainWindow::event(QEvent *e)
{
    // if (!(e->type() == QEvent::UpdateRequest) && !(e->type() == QEvent::MouseMove))
    // {
    //   qDebug() << "[MainWindow] " << e->type();
    // }

    return QWidget::event(e);
}

bool MainWindow::vulkanWindowEvent(QEvent *event)
{
    // qDebug() << "[MainWindow] vulkanWindowEvent: " << event;
    return true;
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

            break;
        }
    }
}

MapView *MainWindow::currentMapView() const noexcept
{
    return mapTabs->currentMapView();
}

bool MainWindow::selectBrush(Brush *brush) noexcept
{
    return _paletteWindow->selectBrush(brush);
}

//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>Initialization>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>

void MainWindow::initializeUI()
{
    mapTabs = new MapTabWidget(this);
    connect(mapTabs, &MapTabWidget::mapTabClosed, this, &MainWindow::mapTabCloseEvent);
    connect(mapTabs, &MapTabWidget::currentChanged, this, &MainWindow::mapTabChangedEvent);

    propertyWindow = new ItemPropertyWindow(QUrl("qrc:/vme/qml/itemPropertyWindow.qml"), this);
    connect(propertyWindow, &ItemPropertyWindow::countChanged, [this](std::variant<Item *, ItemLocation> itemData, int count, bool shouldCommit) {
        MapView &mapView = *currentMapView();
        if (shouldCommit)
        {
            mapView.beginTransaction(TransactionType::ModifyItem);
            if (std::holds_alternative<Item *>(itemData))
            {
                mapView.setItemCount(std::get<Item *>(itemData), count);
            }
            else if (std::holds_alternative<ItemLocation>(itemData))
            {
                mapView.setItemCount(std::get<ItemLocation>(itemData), count);
            }
            else
            {
                ABORT_PROGRAM("mainwindow.cpp : Bad itemData.");
            }

            mapView.endTransaction(TransactionType::ModifyItem);
        }
        else
        {
            if (std::holds_alternative<Item *>(itemData))
            {
                std::get<Item *>(itemData)->setCount(count);
            }
            else if (std::holds_alternative<ItemLocation>(itemData))
            {
                std::get<ItemLocation>(itemData).item(mapView)->setCount(count);
            }
            else
            {
                ABORT_PROGRAM("mainwindow.cpp : Bad itemData.");
            }
        }

        mapView.requestDraw();
    });

    QMenuBar *menu = createMenuBar();
    rootLayout->setMenuBar(menu);

    Splitter *splitter = new Splitter();
    rootLayout->addWidget(splitter, BorderLayout::Position::Center);

    initializePaletteWindow();

    splitter->addWidget(_paletteWindow);
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

    topItemInfo->setText("");
    bottomLayout->addWidget(topItemInfo);

    rootLayout->addWidget(bottomStatusBar, BorderLayout::Position::South);

    setLayout(rootLayout);
}

void MainWindow::initializePaletteWindow()
{
    GuiImageCache::initialize();

    _paletteWindow = new ItemPaletteWindow(&editorAction, this);

    // Raw palette
    {
        ItemPalette &rawPalette = ItemPalettes::createPalette("Raw Palette");

        Tileset *bottomTileset = rawPalette.createTileset("Bottom items");
        Tileset *groundTileset = rawPalette.createTileset("Grounds");
        Tileset *borderTileset = rawPalette.createTileset("Borders");
        Tileset *unsightTileset = rawPalette.createTileset("Sight-blocking");
        Tileset *doorTileset = rawPalette.createTileset("Doors");
        Tileset *archwayTileset = rawPalette.createTileset("Archways");
        Tileset *containerTileset = rawPalette.createTileset("Containers");
        Tileset *hangableTileset = rawPalette.createTileset("Hangables");
        Tileset *pickuableTileset = rawPalette.createTileset("Pickupables");
        Tileset *equipmentTileset = rawPalette.createTileset("Equipment");
        Tileset *lightSourceTileset = rawPalette.createTileset("Light source");

        Tileset *interiorTileset = rawPalette.createTileset("Interior / Wrap / Unwrap");

        Tileset *otherTileset = rawPalette.createTileset("Other");

        int from = 100;
        int to = 40000;

#ifdef _DEBUG_VME
        from = 100;
        to = 5000;
#endif

        for (int i = from; i < to; ++i)
        {
            ItemType *itemType = Items::items.getItemTypeByServerId(i);
            if (!itemType || !itemType->isValid())
                continue;

            if (itemType->hasFlag(AppearanceFlag::Corpse) || itemType->hasFlag(AppearanceFlag::PlayerCorpse))
            {
                // Skip corpses
                continue;
            }
            else if (itemType->hasFlag(AppearanceFlag::Bottom))
            {
                bottomTileset->addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Ground))
            {
                groundTileset->addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Border))
            {
                borderTileset->addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Top))
            {
                archwayTileset->addRawBrush(i);
            }
            else if (itemType->isDoor())
            {
                doorTileset->addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Unsight))
            {
                unsightTileset->addRawBrush(i);
            }
            else if (itemType->isContainer())
            {
                containerTileset->addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Wrap) || itemType->hasFlag(AppearanceFlag::Unwrap) || itemType->isBed())
            {
                interiorTileset->addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Take))
            {
                if (itemType->hasFlag(AppearanceFlag::Clothes))
                {
                    equipmentTileset->addRawBrush(i);
                }
                else
                {
                    pickuableTileset->addRawBrush(i);
                }
            }
            else if (itemType->hasFlag(AppearanceFlag::Light))
            {
                lightSourceTileset->addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Hang))
            {
                hangableTileset->addRawBrush(i);
            }

            else
            {
                otherTileset->addRawBrush(i);
            }

            // GuiImageCache::cachePixmapForServerId(i);
        }

        _paletteWindow->addPalette(ItemPalettes::getByName("Raw Palette"));
    }

    // Terrain palette
    {
        ItemPalette &terrainPalette = ItemPalettes::createPalette("Terrain palette");

        Tileset *groundTileset = terrainPalette.createTileset("Grounds");

        std::vector<WeightedItemId> weightedIds{
            {4526, 300},
            {4527, 10},
            {4528, 25},
            {4529, 25},
            {4530, 25},
            {4531, 15},
            {4532, 25},
            {4533, 25},
            {4534, 15},
            {4535, 25},
            {4536, 25},
            {4537, 25},
            {4538, 20},
            {4539, 20},
            {4540, 20},
            {4541, 20}};

        GroundBrush *grassBrush = Brush::addGroundBrush(GroundBrush(0, "Grass", std::move(weightedIds)));
        groundTileset->addBrush(grassBrush);

        _paletteWindow->addPalette(ItemPalettes::getByName("Terrain palette"));
    }

    connect(_paletteWindow, &ItemPaletteWindow::brushSelectionEvent, [this](Brush *brush) {
        editorAction.setIfUnlocked(MouseAction::MapBrush(brush));
    });

    Brush *testBrush = Brush::getOrCreateRawBrush(446);
    _paletteWindow->selectBrush(testBrush);
}

QMenuBar *MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar;

    // File
    {
        auto fileMenu = menuBar->addMenu(tr("File"));

        auto newMap = new MenuAction(tr("New Map"), Qt::CTRL | Qt::Key_N, this);
        connect(newMap, &QWidgetAction::triggered, [this] { this->addMapTab(); });
        fileMenu->addAction(newMap);

        auto saveMap = new MenuAction(tr("Save"), Qt::CTRL | Qt::Key_S, this);
        connect(saveMap, &QWidgetAction::triggered, [this] { SaveMap::saveMap(*(currentMapView()->map())); });
        fileMenu->addAction(saveMap);

        auto closeMap = new MenuAction(tr("Close"), Qt::CTRL | Qt::Key_W, this);
        connect(closeMap, &QWidgetAction::triggered, mapTabs, &MapTabWidget::removeCurrentTab);
        fileMenu->addAction(closeMap);
    }

    // Edit
    {
        auto editMenu = menuBar->addMenu(tr("Edit"));

        auto undo = new MenuAction(tr("Undo"), Qt::CTRL | Qt::Key_Z, this);
        editMenu->addAction(undo);

        auto redo = new MenuAction(tr("Redo"), Qt::CTRL | Qt::SHIFT | Qt::Key_Z, this);
        editMenu->addAction(redo);

        editMenu->addSeparator();

        auto cut = new MenuAction(tr("Cut"), Qt::CTRL | Qt::Key_X, this);
        editMenu->addAction(cut);

        auto copy = new MenuAction(tr("Copy"), Qt::CTRL | Qt::Key_C, this);
        editMenu->addAction(copy);

        auto paste = new MenuAction(tr("Paste"), Qt::CTRL | Qt::Key_V, this);
        editMenu->addAction(paste);
    }

    // Map
    {
        auto mapMenu = menuBar->addMenu(tr("Map"));

        auto editTowns = new MenuAction(tr("Edit Towns"), Qt::CTRL | Qt::Key_T, this);
        mapMenu->addAction(editTowns);
    }

    // View
    {
        auto viewMenu = menuBar->addMenu(tr("View"));

        auto zoomIn = new MenuAction(tr("Zoom in"), Qt::CTRL | Qt::Key_Plus, this);
        viewMenu->addAction(zoomIn);

        auto zoomOut = new MenuAction(tr("Zoom out"), Qt::CTRL | Qt::Key_Minus, this);
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
        connect(reloadStyles, &QAction::triggered, [=] { QtUtil::qtApp()->loadStyleSheet(":/vme/style/qss/default.qss"); });
        reloadMenu->addAction(reloadStyles);

        QAction *reloadPropertyQml = new QAction(tr("Reload Properties QML"), this);
        reloadPropertyQml->setShortcut(Qt::Key_F5);
        connect(reloadPropertyQml, &QAction::triggered, [=] { propertyWindow->reloadSource(); });
        reloadMenu->addAction(reloadPropertyQml);
    }

    {
        QAction *debug = new QAction(tr("Toggle debug"), this);
        connect(debug, &QAction::triggered, [=] { DEBUG_FLAG_ACTIVE = !DEBUG_FLAG_ACTIVE; });
        menuBar->addAction(debug);
    }

    {
        QAction *runTest = new QAction(tr("Run MapView test"), this);
        connect(runTest, &QAction::triggered, [=] { currentMapView()->perfTest(); });
        menuBar->addAction(runTest);
    }

    return menuBar;
}

void MainWindow::setVulkanInstance(QVulkanInstance *instance)
{
    vulkanInstance = instance;
}
