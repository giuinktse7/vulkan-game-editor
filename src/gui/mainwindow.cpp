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
#include <QStackedLayout>
#include <QVariant>
#include <QVulkanInstance>
#include <QWidget>
#include <QWindow>

#pragma clang diagnostic pop

#include "../../vendor/rollbear-visit/visit.hpp"
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
#include "search_popup.h"
#include "split_widget.h"
#include "vulkan_window.h"

#include "../brushes/creature_brush.h"

MainLayout::MainLayout(QWidget *mainWidget)
    : mainWidget(mainWidget)
{
    setStackingMode(QStackedLayout::StackAll);
    addWidget(mainWidget);
}

void MainLayout::setSearchWidget(SearchPopupWidget *searchWidget)
{
    this->searchWidget = searchWidget;
}

void MainWindow::windowPressEvent(QWindow *window, QMouseEvent *event)
{
    auto mainLayout = static_cast<MainLayout *>(layout());
    if (mainLayout->isSearchVisible() && window != searchPopupWidget->popupView())
    {
        mainLayout->hideSearch();
    }
}

bool MainLayout::isSearchVisible() const noexcept
{
    return searchVisible;
}

void MainLayout::showSearch()
{
    if (searchVisible)
    {
        return;
    }

    searchVisible = true;
    addWidget(searchWidget);
    setCurrentWidget(searchWidget);
    searchWidget->popupView()->focus();
}

void MainLayout::hideSearch()
{
    if (!searchVisible)
    {
        return;
    }

    searchVisible = false;
    removeWidget(searchWidget);
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

    vulkanWindow->getMapView()->onSelectedTileThingClicked<&MainWindow::mapViewSelectedTileThingClicked>(this);

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
}

//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>Events>>>>>>>
//>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>

void MainWindow::mapViewSelectedTileThingClicked(MapView *mapView, const Tile *tile, TileThing tileThing)
{
    auto position = tile->position();
    bool showInPropertyWindow = !(QApplication::keyboardModifiers() & Qt::KeyboardModifier::AltModifier);
    if (showInPropertyWindow)
    {
        rollbear::visit(
            util::overloaded{
                [this, &position, mapView](Item *item) {
                    propertyWindow->focusItem(item, position, *mapView);
                },
                [this, position](const Creature *creature) {
                    // TODO
                },

                [](const auto &arg) {
                }},
            tileThing);
    }
}

void MainWindow::mapViewSelectionChangedEvent(MapView &mapView)
{
    // TODO Handle the case where the selected thing is something other than an item (ex. a creature).
    // All(?) selectable entities should have properties.
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
        auto creature = tile->creature();
        if (creature->hasName())
        {
            auto description = std::format("Monster \"{}\"", creature->name());
            topItemInfo->setText(QString::fromStdString(description));
        }
        else
        {
            auto id = std::to_string(creature->creatureType.looktype());
            topItemInfo->setText("Creature looktype: " + QString::fromStdString(id));
        }
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

VulkanWindow *MainWindow::currentVulkanWindow() const noexcept
{
    return mapTabs->currentVulkanWindow();
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
    registerPropertyItemListeners();

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
        container->setMinimumWidth(240);
        container->setMaximumWidth(240);

        splitter->addWidget(container);
        splitter->setStretchFactor(2, 0);
    }

    splitter->setSizes(QList<int>({200, 760, 240}));

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

    searchPopupWidget = new SearchPopupWidget(this);

    auto mainWidget = new QWidget(this);
    mainWidget->setLayout(rootLayout);

    MainLayout *layout = new MainLayout(mainWidget);
    layout->setSearchWidget(searchPopupWidget);

    connect(searchPopupWidget->popupView(), &SearchPopupView::requestClose, [layout]() {
        layout->hideSearch();
    });

    setLayout(layout);
}

void MainWindow::setSearchVisible(bool visible)
{
    auto mainLayout = static_cast<MainLayout *>(layout());
    if (visible)
    {
        mainLayout->showSearch();
    }
    else
    {
        mainLayout->hideSearch();
    }
}

void MainWindow::registerPropertyItemListeners()
{
    connect(propertyWindow, &ItemPropertyWindow::actionIdChanged, [this](Item *item, int actionId, bool shouldCommit) {
        MapView &mapView = *currentMapView();

        if (shouldCommit)
        {
            mapView.beginTransaction(TransactionType::ModifyItem);
            mapView.setItemActionId(item, actionId);
            mapView.endTransaction(TransactionType::ModifyItem);
        }
        else
        {
            item->setActionId(actionId);
        }

        mapView.requestDraw();
    });

    connect(propertyWindow, &ItemPropertyWindow::subtypeChanged, [this](Item *item, int subtype, bool shouldCommit) {
        MapView &mapView = *currentMapView();

        if (shouldCommit)
        {
            mapView.beginTransaction(TransactionType::ModifyItem);
            mapView.setSubtype(item, subtype);
            mapView.endTransaction(TransactionType::ModifyItem);
        }
        else
        {
            item->setSubtype(subtype);
        }

        mapView.requestDraw();
    });

    connect(propertyWindow, &ItemPropertyWindow::textChanged, [this](Item *item, const std::string &text) {
        MapView &mapView = *currentMapView();

        mapView.beginTransaction(TransactionType::ModifyItem);
        mapView.setText(item, text);
        mapView.endTransaction(TransactionType::ModifyItem);
    });
}

void MainWindow::initializePaletteWindow()
{
    GuiImageCache::initialize();

    _paletteWindow = new ItemPaletteWindow(&editorAction, this);

    // Raw palette
    {
        ItemPalette &rawPalette = *ItemPalettes::getById("raw");

        auto &bottomTileset = rawPalette.addTileset(Tileset("bottom_items", "Bottom items"));
        // auto &groundTileset = rawPalette.addTileset(Tileset("grounds", "Grounds"));
        auto &borderTileset = rawPalette.addTileset(Tileset("borders", "Borders"));
        auto &unsightTileset = rawPalette.addTileset(Tileset("sight_blocking", "Sight-blocking"));
        auto &doorTileset = rawPalette.addTileset(Tileset("doors", "Doors"));
        auto &archwayTileset = rawPalette.addTileset(Tileset("archways", "Archways"));
        auto &containerTileset = rawPalette.addTileset(Tileset("containers", "Containers"));
        auto &hangableTileset = rawPalette.addTileset(Tileset("hangables", "Hangables"));
        auto &pickuableTileset = rawPalette.addTileset(Tileset("pickupables", "Pickupables"));
        auto &equipmentTileset = rawPalette.addTileset(Tileset("equipment", "Equipment"));
        auto &lightSourceTileset = rawPalette.addTileset(Tileset("light_source", "Light source"));
        auto &interiorTileset = rawPalette.addTileset(Tileset("interior_wrap_unwrap", "Interior / Wrap / Unwrap"));
        auto &otherTileset = rawPalette.addTileset(Tileset("other", "Other"));
        auto &allTileset = rawPalette.addTileset(Tileset("all", "All"));

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

            allTileset.addRawBrush(i);

            if (itemType->hasFlag(AppearanceFlag::Bottom))
            {
                bottomTileset.addRawBrush(i);
            }
            // else if (itemType->hasFlag(AppearanceFlag::Ground))
            // {
            //     groundTileset.addRawBrush(i);
            // }
            else if (itemType->hasFlag(AppearanceFlag::Border))
            {
                borderTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Top))
            {
                archwayTileset.addRawBrush(i);
            }
            else if (itemType->isDoor())
            {
                doorTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Unsight))
            {
                unsightTileset.addRawBrush(i);
            }
            else if (itemType->isContainer())
            {
                containerTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Wrap) || itemType->hasFlag(AppearanceFlag::Unwrap) || itemType->isBed())
            {
                interiorTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Take))
            {
                if (itemType->hasFlag(AppearanceFlag::Clothes))
                {
                    equipmentTileset.addRawBrush(i);
                }
                else
                {
                    pickuableTileset.addRawBrush(i);
                }
            }
            else if (itemType->hasFlag(AppearanceFlag::Light))
            {
                lightSourceTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Hang))
            {
                hangableTileset.addRawBrush(i);
            }
            else
            {
                if (!itemType->hasFlag(AppearanceFlag::Ground))
                {
                    otherTileset.addRawBrush(i);
                }
            }

            // GuiImageCache::cachePixmapForServerId(i);
        }
    }

    // Terrain palette
    {
        ItemPalette &terrainPalette = *ItemPalettes::getById("terrain");

        auto &groundTileset = terrainPalette.addTileset(Tileset("grounds", "Grounds"));

        auto grassBrush = Brush::getGroundBrush("normal_grass");
        if (grassBrush)
        {
            groundTileset.addBrush(grassBrush);
        }

        auto driedGrassBrush = Brush::getGroundBrush("dried_grass");
        if (driedGrassBrush)
        {
            groundTileset.addBrush(driedGrassBrush);
        }

        auto rockSoilBrush = Brush::getGroundBrush("rock_soil");
        if (rockSoilBrush)
        {
            groundTileset.addBrush(rockSoilBrush);
        }
    }

    // Creature palette
    {
        ItemPalette &creaturePalette = *ItemPalettes::getById("creature");

        auto &goblinTileset = creaturePalette.addTileset(Tileset("goblin", "Goblins"));

        // for (int i = 0; i < 1400; ++i)
        // {
        //     auto creatureType = Creatures::creatureType(i);
        //     if (creatureType)
        //     {
        //         goblinTileset.addBrush(new CreatureBrush(std::to_string(i), i));
        //     }
        // }

        const auto addTestCreatureBrush = [&goblinTileset](std::string id, std::string name, int looktype) {
            goblinTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(id, name, looktype))));
        };

        goblinTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(
            "colorful_nomad",
            "Colorful Nomad",
            Outfit(146, 116, 68, 68, 68)))));

        goblinTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(
            "nomad",
            "Nomad",
            Outfit(146, 114, 20, 22, 2)))));

        // addTestCreatureBrush("nomad", "Nomad", 146);
        addTestCreatureBrush("rat", "Rat", 21);
        addTestCreatureBrush("bear", "Bear", 16);
        addTestCreatureBrush("cyclops", "Cyclops", 22);
        addTestCreatureBrush("goblin", "Goblin", 61);
        addTestCreatureBrush("goblin_scavenger", "Goblin Scavenger", 297);
        addTestCreatureBrush("goblin_assassin", "Goblin Assassin", 296);
    }

    for (auto &[k, palette] : ItemPalettes::itemPalettes())
    {
        _paletteWindow->addPalette(&palette);
    }

    connect(_paletteWindow, &ItemPaletteWindow::brushSelectionEvent, [this](Brush *brush) {
        editorAction.setIfUnlocked(MouseAction::MapBrush(brush));
    });
    // Brush *testBrush = Brush::getOrCreateRawBrush(2016);
    // Brush *testBrush = Brush::getOrCreateRawBrush(2005);
    Brush *testBrush = Brush::getOrCreateRawBrush(1892);
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
        connect(undo, &QWidgetAction::triggered, [this] { this->currentMapView()->undo(); });
        editMenu->addAction(undo);

        auto redo = new MenuAction(tr("Redo"), Qt::CTRL | Qt::SHIFT | Qt::Key_Z, this);
        connect(redo, &QWidgetAction::triggered, [this] { this->currentMapView()->redo(); });
        editMenu->addAction(redo);

        editMenu->addSeparator();

        auto jumpToBrush = new MenuAction(tr("Jump to brush..."), Qt::Key_J, this);
        connect(jumpToBrush, &QWidgetAction::triggered, [this] {
            auto mainLayout = static_cast<MainLayout *>(layout());
            setSearchVisible(!mainLayout->isSearchVisible());
        });
        editMenu->addAction(jumpToBrush);

        auto temp = new MenuAction(tr("Temp debug"), Qt::Key_K, this);
        connect(temp, &QWidgetAction::triggered, [this] {
            VME_LOG_D(".");
        });
        editMenu->addAction(temp);

        editMenu->addSeparator();

        auto cut = new MenuAction(tr("Cut"), Qt::CTRL | Qt::Key_X, this);
        editMenu->addAction(cut);

        auto copy = new MenuAction(tr("Copy"), Qt::CTRL | Qt::Key_C, this);
        connect(copy, &QWidgetAction::triggered, [this] { this->mapCopyBuffer.copySelection(*this->currentMapView()); });
        editMenu->addAction(copy);

        auto paste = new MenuAction(tr("Paste"), Qt::CTRL | Qt::Key_V, this);
        connect(paste, &QWidgetAction::triggered, [this] {
            if (this->mapCopyBuffer.empty() || !this->currentMapView())
            {
                return;
            }

            this->editorAction.set(MouseAction::PasteMapBuffer(&this->mapCopyBuffer));
            this->currentMapView()->requestDraw();
        });
        editMenu->addAction(paste);

        // Brush actions
        editMenu->addSeparator();

        auto rotateBrush = new MenuAction(tr("Rotate brush"), Qt::Key_R, this);
        connect(rotateBrush, &QWidgetAction::triggered, [this] { this->currentMapView()->rotateBrush(); });
        editMenu->addAction(rotateBrush);
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
        connect(reloadPropertyQml, &QAction::triggered, [=] {
            propertyWindow->reloadSource();
            searchPopupWidget->popupView()->reloadSource();
        });
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
