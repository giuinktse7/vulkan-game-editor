#include "mainwindow.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonportable-include-path"
#endif

#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QQuickView>
#include <QShortcut>
#include <QSlider>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QVariant>
#include <QVulkanInstance>
#include <QWidget>
#include <QWindow>

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "../../vendor/rollbear-visit/visit.hpp"
#include "../brushes/border_brush.h"
#include "../brushes/creature_brush.h"
#include "../brushes/doodad_brush.h"
#include "../brushes/ground_brush.h"
#include "../brushes/mountain_brush.h"
#include "../brushes/wall_brush.h"
#include "../error.h"
#include "../graphics/appearance_types.h"
#include "../item_location.h"
#include "../load_map.h"
#include "../qt/logging.h"
#include "../save_map.h"
#include "../settings.h"
#include "../util.h"
#include "border_layout.h"
#include "gui_thing_image.h"
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

void MainWindow::addMapTab(const std::filesystem::path &path)
{
    Map map = LoadMap::loadMap(path);
    std::shared_ptr<Map> sharedMap = std::make_shared<Map>(std::move(map));
    addMapTab(sharedMap);
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
      rootLayout(new QVBoxLayout),
      positionStatus(new QLabel),
      zoomStatus(new QLabel),
      topItemInfo(new QLabel),
      _minimapWidget(new MinimapWidget(this)),
      lastUiToggleTime(TimePoint::now()) {}

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

                [this, &position, mapView](Creature *creature) {
                    propertyWindow->focusCreature(creature, position, *mapView);
                },

                [](const auto &arg) {
                }},
            tileThing);
    }
}

void MainWindow::mapViewSelectionChangedEvent(MapView &mapView)
{
    if (mapView.singleThingSelected())
    {
        bool showInPropertyWindow = !(QApplication::keyboardModifiers() & Qt::KeyboardModifier::AltModifier);
        if (showInPropertyWindow)
        {
            Tile *tile = mapView.singleSelectedTile();
            auto position = tile->position();

            TileThing thing = tile->firstSelectedThing();
            rollbear::visit(
                util::overloaded{
                    [this, &position, &mapView](Item *item) {
                        propertyWindow->focusItem(item, position, mapView);
                    },

                    [this, &position, &mapView](Creature *creature) {
                        propertyWindow->focusCreature(creature, position, mapView);
                    },

                    [](const auto &arg) {}},
                thing);
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
    //     qDebug() << "[MainWindow] " << e->type();
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
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->setMenuBar(menu);

    Splitter *splitter = new Splitter();
    rootLayout->addWidget(splitter);

    initializePaletteWindow();

    splitter->addWidget(_paletteWindow);
    splitter->addWidget(mapTabs);
    splitter->setStretchFactor(1, 1);
    splitter->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding));

    {
        propertyWindowContainer = propertyWindow->wrapInWidget();
        propertyWindowContainer->setMinimumWidth(240);
        propertyWindowContainer->setMaximumWidth(240);

        splitter->addWidget(propertyWindowContainer);
        splitter->setStretchFactor(2, 0);
    }

    splitter->setSizes(QList<int>({200, 760, 240}));

    QWidget *bottomStatusBar = new QWidget;
    bottomStatusBar->setSizePolicy(QSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum));

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomStatusBar->setLayout(bottomLayout);
    bottomLayout->setContentsMargins(14, 4, 14, 4);
    bottomLayout->setSpacing(0);

    positionStatus->setText("");
    bottomLayout->addWidget(positionStatus);

    zoomStatus->setText("");
    bottomLayout->addWidget(zoomStatus);

    topItemInfo->setText("");
    bottomLayout->addWidget(topItemInfo);

    rootLayout->addWidget(bottomStatusBar);

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

    connect(propertyWindow, &ItemPropertyWindow::spawnIntervalChanged, [this](Creature *creature, int spawnInterval, bool shouldCommit) {
        MapView &mapView = *currentMapView();

        if (shouldCommit)
        {
            mapView.beginTransaction(TransactionType::ModifyCreature);
            mapView.setSpawnInterval(creature, spawnInterval);
            mapView.endTransaction(TransactionType::ModifyCreature);
        }
        else
        {
            creature->setSpawnInterval(spawnInterval);
        }
    });
}

void MainWindow::initializePaletteWindow()
{
    GUIImageCache::initialize();

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
        int to = 45000;

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

        // Grounds
        {
            auto &groundTileset = terrainPalette.addTileset(Tileset("grounds", "Grounds"));
            auto &brushes = Brush::getGroundBrushes();
            for (auto &brush : brushes)
            {
                groundTileset.addBrush(brush.second.get());
            }

            auto &mountainBrushes = Brush::getMountainBrushes();
            for (auto &brush : mountainBrushes)
            {
                groundTileset.addBrush(brush.second.get());
            }
        }

        // Borders
        {
            auto &borderTileset = terrainPalette.addTileset(Tileset("borders", "Borders"));
            auto &brushes = Brush::getBorderBrushes();
            for (auto &brush : brushes)
            {
                borderTileset.addBrush(brush.second.get());
            }
        }

        // Walls
        {
            auto &wallTileset = terrainPalette.addTileset(Tileset("walls", "Walls"));
            auto &brushes = Brush::getWallBrushes();
            for (auto &brush : brushes)
            {
                wallTileset.addBrush(brush.second.get());
            }
        }

        // Doodads
        {
            auto &doodadTileset = terrainPalette.addTileset(Tileset("doodads", "Doodads"));
            auto &brushes = Brush::getDoodadBrushes();
            for (auto &brush : brushes)
            {
                doodadTileset.addBrush(brush.second.get());
            }
        }
    }

    // Creature palette
    {
        ItemPalette &creaturePalette = *ItemPalettes::getById("creature");

        auto &otherTileset = creaturePalette.addTileset(Tileset("other", "Other"));

        // for (int i = 0; i < 1400; ++i)
        // {
        //     auto creatureType = Creatures::creatureType(i);
        //     if (creatureType)
        //     {
        //         goblinTileset.addBrush(new CreatureBrush(std::to_string(i), i));
        //     }
        // }

        const auto addTestCreatureBrush = [&otherTileset](std::string id, std::string name, int looktype) {
            otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(id, name, looktype))));
        };

        otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(
            "colorful_nomad",
            "Colorful Nomad",
            // Outfit(150, 116, 68, 68, 68, Outfit::Addon::First | Outfit::Addon::Second, 370)))));
            Outfit(150, 116, 68, 68, 68, Outfit::Addon::None, 370)))));
        // Outfit(150, 116, 68, 68, 68, Outfit::Addon::None)))));

        otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(
            "colorful_nomad_2",
            "Colorful Nomad 2",
            // Outfit(150, 116, 68, 68, 68, Outfit::Addon::First | Outfit::Addon::Second, 370)))));
            Outfit(150, 116, 68, 68, 68, Outfit::Addon::None)))));

        otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(
            "nomad",
            "Nomad",
            Outfit(146, 114, 20, 22, 2)))));

        addTestCreatureBrush("rat", "Rat", 21);
        addTestCreatureBrush("bear", "Bear", 16);
        addTestCreatureBrush("cyclops", "Cyclops", 22);
        addTestCreatureBrush("goblin", "Goblin", 61);
        addTestCreatureBrush("goblin_scavenger", "Goblin Scavenger", 297);
        addTestCreatureBrush("goblin_assassin", "Goblin Assassin", 296);
        addTestCreatureBrush("squirrel", "Squirrel", 274);
        addTestCreatureBrush("cobra", "Cobra", 81);

        otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType("mimic", "Mimic", Outfit::fromServerId(1740)))));
    }

    for (auto &[k, palette] : ItemPalettes::itemPalettes())
    {
        _paletteWindow->addPalette(&palette);
    }

    connect(_paletteWindow, &ItemPaletteWindow::brushSelectionEvent, [this](Brush *brush) {
        editorAction.setIfUnlocked(MouseAction::MapBrush(brush));
        brush->updatePreview(0);
    });
}

QMenuBar *MainWindow::createMenuBar()
{
    QMenuBar *menuBar = new QMenuBar;

    const auto addMenuItem = [this](QMenu *menu, const char *text, const QKeySequence shortcut = 0, std::function<void()> f = nullptr) {
        auto action = new MenuAction(tr(text), shortcut, menu);
        if (f)
        {
            connect(action, &QWidgetAction::triggered, f);
        }
        menu->addAction(action);
        return action;
    };

    // File
    {
        auto fileMenu = menuBar->addMenu(tr("File"));

        addMenuItem(fileMenu, "New Map", Qt::CTRL | Qt::Key_N, [this] { this->addMapTab(); });
        addMenuItem(fileMenu, "Save", Qt::CTRL | Qt::Key_S, [this] { SaveMap::saveMap(*(currentMapView()->map())); });
        addMenuItem(fileMenu, "Close", Qt::CTRL | Qt::Key_W, [this] { mapTabs->removeCurrentTab(); });
    }

    // Edit
    {
        auto editMenu = menuBar->addMenu(tr("Edit"));

        addMenuItem(editMenu, "Undo", Qt::CTRL | Qt::Key_Z, [this] { this->currentMapView()->undo(); });
        addMenuItem(editMenu, "Redo", Qt::CTRL | Qt::SHIFT | Qt::Key_Z, [this] { this->currentMapView()->redo(); });

        addMenuItem(editMenu, "Jump To Brush...", Qt::Key_J, [this] {
            auto mainLayout = static_cast<MainLayout *>(layout());
            setSearchVisible(!mainLayout->isSearchVisible());
        });

        addMenuItem(editMenu, "Temp Debug", Qt::Key_K, [this] { VME_LOG("."); });
        editMenu->addAction(new MenuSeparator(this));

        addMenuItem(editMenu, "Cut", Qt::CTRL | Qt::Key_X, [this] {
            MapView *mapView = this->currentMapView();
            if (mapView)
            {
                this->mapCopyBuffer.copySelection(*mapView);
                mapView->deleteSelectedItems();
            }
        });
        addMenuItem(editMenu, "Copy", Qt::CTRL | Qt::Key_C, [this] {
            this->mapCopyBuffer.copySelection(*this->currentMapView());
        });
        addMenuItem(editMenu, "Paste", Qt::CTRL | Qt::Key_V, [this] {
            if (this->mapCopyBuffer.empty() || !this->currentMapView())
            {
                return;
            }

            this->editorAction.set(MouseAction::PasteMapBuffer(&this->mapCopyBuffer));
            this->currentMapView()->requestDraw();
        });

        auto eyeDropper = new QShortcut(Qt::Key_I, this);
        connect(eyeDropper, &QShortcut::activated, [this]() {
            MapView *mapView = currentMapView();
            if (mapView->underMouse())
            {
                currentVulkanWindow()->eyedrop(mapView->mouseGamePos());
            }
        });

        editMenu->addAction(new MenuSeparator(this));

        // Brush actions
        addMenuItem(editMenu, "Previous Brush Variation", Qt::Key_R, [this] {
            this->currentMapView()->rotateBrush(false);
        });

        // Brush actions
        addMenuItem(editMenu, "Next Brush Variation", Qt::Key_T, [this] {
            this->currentMapView()->rotateBrush(true);
        });

        auto brushOptionMenu = editMenu->addMenu(tr("Border Options"));

        addMenuItem(brushOptionMenu, "Cycle Border Mode", Qt::Key_B, [this] {
            auto action = this->editorAction.as<MouseAction::MapBrush>();
            if (action)
            {
                if (action->brush->type() == BrushType::Border)
                {
                    auto brushType = Settings::BORDER_BRUSH_VARIATION == BorderBrushVariationType::General
                                         ? BorderBrushVariationType::Detailed
                                         : BorderBrushVariationType::General;

                    BorderBrush::setBrushVariation(brushType);
                    this->currentMapView()->requestDraw();
                }
                else if (action->brush->type() == BrushType::Mountain)
                {
                    Settings::PLACE_MOUNTAIN_FEATURES = !Settings::PLACE_MOUNTAIN_FEATURES;
                }
            }
        });

        addMenuItem(brushOptionMenu, "Toggle Automatic Bordering", Qt::Key_A, [this] {
            Settings::AUTO_BORDER = !Settings::AUTO_BORDER;
        });
    }

    // Map
    {
        auto mapMenu = menuBar->addMenu(tr("Map"));

        addMenuItem(mapMenu, "Edit Towns", Qt::CTRL | Qt::Key_T, []() {});
    }

    // View
    {
        auto viewMenu = menuBar->addMenu(tr("View"));

        addMenuItem(viewMenu, "Zoom In", Qt::CTRL | Qt::Key_Plus, []() {});
        addMenuItem(viewMenu, "Zoom Out", Qt::CTRL | Qt::Key_Minus, []() {});

        auto pan = new QShortcut(Qt::Key_Space, this);
        connect(pan, &QShortcut::activated, [this]() {
            auto mapView = currentMapView();
            bool panning = mapView->editorAction.is<MouseAction::Pan>();
            if (panning || QApplication::mouseButtons() & Qt::MouseButton::LeftButton)
            {
                return;
            }

            if (mapView->underMouse())
            {
                currentVulkanWindow()->setCursor(Qt::OpenHandCursor);
            }

            MouseAction::Pan panAction;
            mapView->editorAction.setIfUnlocked(panAction);
            mapView->requestDraw();

            this->currentVulkanWindow()->requestActivate();
        });

        // Toggle property panel
        addMenuItem(viewMenu, "Toggle Property Panel", Qt::CTRL | Qt::Key_Q, [this] {
            if (this->lastUiToggleTime.elapsedMillis() > Settings::UI_CHANGE_TIME_DELAY_MILLIS)
            {
                this->lastUiToggleTime = TimePoint::now();
                setPropertyPanelVisible(!propertyWindowContainer->isVisible());
            }
        });

        addMenuItem(viewMenu, "Show animations", Qt::Key_L, [this]() {
            Settings::RENDER_ANIMATIONS = !Settings::RENDER_ANIMATIONS;

            // Re-render if necessary
            if (Settings::RENDER_ANIMATIONS)
            {
                MapView *mapView = currentMapView();
                if (mapView)
                {
                    mapView->requestDraw();
                }
            }
        });
    }

    // Window
    {
        auto windowMenu = menuBar->addMenu(tr("Window"));
        addMenuItem(windowMenu, "Minimap", Qt::Key_M, [this]() { this->_minimapWidget->toggle(); });
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

        addMenuItem(reloadMenu, "Reload Styles", 0, []() { QtUtil::qtApp()->loadStyleSheet(":/vme/style/qss/default.qss"); });

        addMenuItem(reloadMenu, "Reload Properties QML", Qt::Key_F5, [this]() {
            propertyWindow->reloadSource();
            searchPopupWidget->popupView()->reloadSource();
        });
    }

    {
        QAction *debug = new QAction(tr("Toggle Debug"), this);
        connect(debug, &QAction::triggered, [=] { DEBUG_FLAG_ACTIVE = !DEBUG_FLAG_ACTIVE; });
        menuBar->addAction(debug);
    }

    {
        QAction *runBorderTest = new QAction(tr("Run Border Test"), this);
        connect(runBorderTest, &QAction::triggered, [=] { currentMapView()->testBordering(); });
        menuBar->addAction(runBorderTest);
    }

    {
        QAction *runTest = new QAction(tr("Run MapView Test"), this);
        connect(runTest, &QAction::triggered, [=] { currentMapView()->perfTest(); });
        menuBar->addAction(runTest);
    }

    return menuBar;
}

void MainWindow::setPropertyPanelVisible(bool visible)
{
    if (visible)
    {
        propertyWindowContainer->show();
    }
    else
    {
        propertyWindowContainer->hide();
    }
}

void MainWindow::requestMinimapUpdate()
{
    if (_minimapWidget->isVisible())
    {
        _minimapWidget->update();
    }
}

void MainWindow::setVulkanInstance(QVulkanInstance *instance)
{
    vulkanInstance = instance;
}

void MainWindow::copySelection()
{
    MapView *mapView = currentMapView();
    if (mapView)
    {
        mapCopyBuffer.copySelection(*mapView);
    }
}

bool MainWindow::hasCopyBuffer() const
{
    return !mapCopyBuffer.empty();
}

MapCopyBuffer &MainWindow::getMapCopyBuffer()
{
    return mapCopyBuffer;
}