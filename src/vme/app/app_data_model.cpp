#include "app_data_model.h"

#include <QGuiApplication>
#include <QQmlEngine>

#include "combobox_model.h"
#include "context_menu_model.h"
#include "core/item_palette.h"
#include "core/logger.h"
#include "core/tileset.h"
#include "tileset_model.h"

#include "core/brushes/border_brush.h"
#include "core/brushes/brush.h"
#include "core/brushes/creature_brush.h"
#include "core/brushes/doodad_brush.h"
#include "core/brushes/ground_brush.h"
#include "core/brushes/mountain_brush.h"
#include "core/brushes/raw_brush.h"
#include "core/brushes/wall_brush.h"
#include "core/load_map.h"
#include "core/save_map.h"
#include "core/settings.h"

#include <memory>
#include <ranges>

ItemPaletteViewData::ItemPaletteViewData(ComboBoxModel *paletteDropdownModel,
                                         ComboBoxModel *tilesetDropdownModel,
                                         QObject *brushList,
                                         TileSetModel *tilesetModel)
    : paletteDropdownModel(paletteDropdownModel),
      tilesetDropdownModel(tilesetDropdownModel),
      brushList(brushList),
      tilesetModel(tilesetModel)
{
}

void ItemPaletteViewData::positionViewAtIndex(int index) const
{
    VME_LOG_D("ItemPaletteViewData::positionViewAtIndex: " << index);
    emit tilesetModel->brushIndexSelected(index);
    // QMetaObject::invokeMethod(brushList, "positionViewAtIndex", Qt::QueuedConnection, Q_ARG(int, index));
}

AppDataModel::AppDataModel()
    : _townListModel(std::make_unique<TownListModel>()), _filteredSearchModel(std::make_unique<FilteredSearchModel>())
{
    _filteredSearchModel->setSourceModel(&this->searchResultModel);
    // Do not pass ownership to QML. If QML assumes ownership, it will attempt to delete the object when it is no longer used.
    QQmlEngine::setObjectOwnership(_townListModel.get(), QQmlEngine::CppOwnership);

    addMapTab("Untitled-1");
    addMapTab("Untitled-2");
}

void AppDataModel::mapTabSelected(int prevIndex, int index)
{
    auto tabModel = mapTabs();

    if (tabModel->empty())
    {
        return;
    }

    if (prevIndex < tabModel->size() && prevIndex != -1)
    {
        QmlMapItem *item = mapTabs()->get(prevIndex).item;
        if (item)
        {
            item->setActive(false);
        }
    }

    if (index < tabModel->size() && index != -1)
    {
        QmlMapItem *item = mapTabs()->get(index).item;
        if (item)
        {
            item->setActive(true);

            // Update town list model
            auto mapView = currentMapView();
            if (mapView)
            {
                _townListModel->setMap(mapView->mapAsShared());
            }
            else
            {
                _townListModel->clear();
            }
        }
    }
}

void AppDataModel::addMapTab(std::string tabName)
{
    mapTabs()->addTab(tabName);
}

void AppDataModel::removeMapTab(int index)
{
    mapTabs()->removeTab(index);
}

MapView *AppDataModel::currentMapView()
{
    if (mapTabs()->empty())
    {
        return nullptr;
    }

    auto item = mapTabs()->get(_currentMapIndex).item;

    return item ? item->mapView.get() : nullptr;
}

void AppDataModel::initializeItemPalettePanel(ComboBoxModel *paletteDropdownModel, ComboBoxModel *tilesetDropdownModel, QObject *brushList, TileSetModel *tilesetModel)
{
    paletteDropdownModel->setData(ItemPalettes::getItemPaletteList());

    itemPalettes.emplace_back(paletteDropdownModel,
                              tilesetDropdownModel,
                              brushList,
                              tilesetModel);
}

void AppDataModel::destroyItemPalettePanel(ComboBoxModel *paletteModel)
{
    auto removed = std::remove_if(
        itemPalettes.begin(),
        itemPalettes.end(),
        [paletteModel](const ItemPaletteViewData &itemPalette) {
            return itemPalette.paletteDropdownModel == paletteModel;
        });

    itemPalettes.erase(removed, itemPalettes.end());
}

void AppDataModel::setItemPalette(ComboBoxModel *model, QString paletteId)
{
    auto palette = ItemPalettes::getById(paletteId.toStdString());
    if (palette)
    {
        auto result = palette->tilesets() | std::views::transform([](const std::unique_ptr<Tileset> &x) {
                          return NamedId{.id = x->id(), .name = x->name()};
                      });

        auto data = std::vector<NamedId>{result.begin(), result.end()};
        model->setData(data);
    }
    else
    {
        VME_LOG("[WARN] Could not find palette with id '" << paletteId.toStdString() << "'.");
    }
}

void AppDataModel::setTileset(TileSetModel *model, QString paletteId, QString tilesetId)
{
    auto palette = ItemPalettes::getById(paletteId.toStdString());
    if (palette)
    {
        auto tileset = palette->getTileset(tilesetId.toStdString());
        if (tileset)
        {
            model->setTileset(tileset);
        }
    }
}

void AppDataModel::populateRightClickContextMenu(ContextMenuModel *model)
{
    auto mapView = currentMapView();

    auto mouseGamePos = mapView->mouseGamePos();
    _contextMenuPosition = mouseGamePos;
    Tile *tile = mapView->getTile(mouseGamePos);

    std::vector<ContextMenuModel::ContextMenuItem> actions;

    if (tile)
    {
        mapView->selectTopThing(mouseGamePos);

        actions.push_back({.text = "Cut", .option = static_cast<int>(ContextMenuOption::Cut), .enabled = true});
        actions.push_back({.text = "Copy", .option = static_cast<int>(ContextMenuOption::Copy), .enabled = true});
        actions.push_back({.text = "Copy Position", .option = static_cast<int>(ContextMenuOption::CopyPosition), .enabled = true});
        actions.push_back({.text = "Paste", .option = static_cast<int>(ContextMenuOption::Paste), .enabled = !mapCopyBuffer.empty()});
        actions.push_back({.text = "Delete", .option = static_cast<int>(ContextMenuOption::Delete), .enabled = true});

        auto brushes = tile->getTopBrushes();
        for (int i = 0; i < brushes.size(); ++i)
        {
            auto brush = brushes[i];
            ContextMenuOption optionType;
            switch (brush->type())
            {
                case BrushType::Raw:
                    optionType = ContextMenuOption::SelectRawBrush;
                    break;
                case BrushType::Creature:
                    optionType = ContextMenuOption::SelectCreatureBrush;
                    break;
                case BrushType::Ground:
                    optionType = ContextMenuOption::SelectGroundBrush;
                    break;
                case BrushType::Border:
                    optionType = ContextMenuOption::SelectBorderBrush;
                    break;
                case BrushType::Doodad:
                    optionType = ContextMenuOption::SelectDoodadBrush;
                    break;
                case BrushType::Wall:
                    optionType = ContextMenuOption::SelectWallBrush;
                    break;
                case BrushType::Mountain:
                    optionType = ContextMenuOption::SelectMountainBrush;
                    break;
                default:
                    VME_LOG_ERROR("Unknown brush type: " << static_cast<int>(brush->type()) << " for brush: " << brush->name());
            }
            actions.push_back({.text = brush->name(), .option = static_cast<int>(optionType), .enabled = true});
        }
    }
    else
    {
        mapView->commitTransaction(TransactionType::Selection, [this, mapView] { mapView->clearSelection(); });
        actions.push_back({.text = "Cut", .option = static_cast<int>(ContextMenuOption::Cut), .enabled = false});
        actions.push_back({.text = "Copy", .option = static_cast<int>(ContextMenuOption::Copy), .enabled = false});
        actions.push_back({.text = "Copy Position", .option = static_cast<int>(ContextMenuOption::CopyPosition), .enabled = true});
        actions.push_back({.text = "Paste", .option = static_cast<int>(ContextMenuOption::Paste), .enabled = !mapCopyBuffer.empty()});
        actions.push_back({.text = "Delete", .option = static_cast<int>(ContextMenuOption::Delete), .enabled = false});
    }

    model->setData(actions);

    mapView->editorAction.reset();
}

void AppDataModel::onContextMenuAction(int id)
{
#define SELECT_BRUSH(__method)                                                                                              \
    {                                                                                                                       \
        if (_contextMenuPosition)                                                                                           \
        {                                                                                                                   \
            Tile *tile = mapView->getTile(_contextMenuPosition.value());                                                    \
            if (tile)                                                                                                       \
            {                                                                                                               \
                auto brush = Brush::__method(*tile);                                                                        \
                if (brush)                                                                                                  \
                {                                                                                                           \
                    mapView->commitTransaction(TransactionType::Selection, [this, mapView] { mapView->clearSelection(); }); \
                    this->selectBrush(brush);                                                                               \
                }                                                                                                           \
            }                                                                                                               \
        }                                                                                                                   \
    }

    auto mapView = currentMapView();
    if (!mapView)
    {
        return;
    }

    switch (static_cast<ContextMenuOption>(id))
    {
        case ContextMenuOption::Copy:
            mapCopyBuffer.copySelection(*mapView);
            break;
        case ContextMenuOption::Paste:
            if (!mapCopyBuffer.empty() && mapView)
            {
                mapView->editorAction.set(MouseAction::PasteMapBuffer(&mapCopyBuffer));
                mapView->requestDraw();
            }
            break;
        case ContextMenuOption::Cut:
            mapCopyBuffer.copySelection(*mapView);
            mapView->deleteSelectedItems();
            break;
        case ContextMenuOption::Delete:
            mapView->deleteSelectedItems();
            break;
        case ContextMenuOption::CopyPosition:
            // TODO
            break;
        case ContextMenuOption::SelectRawBrush:
            SELECT_BRUSH(getRawBrush)
            break;
        case ContextMenuOption::SelectGroundBrush:
            SELECT_BRUSH(getGroundBrush)
            break;
        case ContextMenuOption::SelectDoodadBrush:
            SELECT_BRUSH(getDoodadBrush)
            break;
        case ContextMenuOption::SelectMountainBrush:
            SELECT_BRUSH(getMountainBrush)
            break;
        case ContextMenuOption::SelectWallBrush:
            SELECT_BRUSH(getWallBrush)
            break;
        case ContextMenuOption::SelectBorderBrush:
            SELECT_BRUSH(getBorderBrush)
            break;
        case ContextMenuOption::SelectCreatureBrush:
            SELECT_BRUSH(getCreatureBrush)
            break;
        default:
            VME_LOG_ERROR("Unknown context menu action with ID: " << id);
    }

#undef SELECT_BRUSH

    _contextMenuPosition.reset();
}

void AppDataModel::selectBrush(TileSetModel *model, int index)
{
    Brush *brush = model->getBrush(index);

    if (brush)
    {
        selectBrush(brush);
    }
    else
    {
        VME_LOG_D("[WARN] Attempted to select non-existent brush at index " << index << " for model '" << model << "'.");
    }
}

void AppDataModel::selectBrush(Brush *brush, bool showInItemPalette)
{
    VME_LOG_D("AppDataModel::selectBrush: " << brush->name());
    auto mapView = currentMapView();

    EditorAction::editorAction.setBrush(brush);

    if (showInItemPalette)
    {
        bool positioned = false;
        for (const auto paletteData : itemPalettes)
        {
            // TODO Can be made faster because the brush knows which tileset it belongs to
            int index = paletteData.tilesetModel->indexOfBrush(brush);
            if (index != -1)
            {
                VME_LOG_D("Found in open palette at index " << index);
                positioned = true;
                paletteData.positionViewAtIndex(index);
                break;
            }
        }

        if (!positioned && !itemPalettes.empty())
        {
            // Show the brush in the first palette
            auto paletteData = itemPalettes[0];

            // Use tileset from brush if available
            auto tileset = brush->tileset();
            if (tileset)
            {
                VME_LOG_D("Tileset: " << tileset->name());
                auto palette = tileset->palette();
                if (palette)
                {
                    VME_LOG_D("Palette: " << palette->name());
                    paletteData.paletteDropdownModel->setSelectedId(palette->id());
                    paletteData.tilesetDropdownModel->setSelectedId(tileset->id());

                    int index = tileset->indexOf(brush);
                    VME_LOG_D("Index: " << index);

                    paletteData.positionViewAtIndex(index);
                }
            }
        }
    }

    mapView->requestDraw();
}

void AppDataModel::setcurrentMapIndex(int index)
{
    if (_currentMapIndex != index)
    {
        int prevIndex = _currentMapIndex;
        VME_LOG("AppDataModel::setcurrentMapIndex: " << index);
        _currentMapIndex = index;
        emit currentMapIndexChanged(index);

        mapTabSelected(prevIndex, index);
    }
}

void AppDataModel::copy()
{
    MapView *mapView = currentMapView();
    if (mapView)
    {
        mapCopyBuffer.copySelection(*mapView);
    }
}

void AppDataModel::paste()
{
    if (mapCopyBuffer.empty() || !currentMapView())
    {
        return;
    }

    EditorAction::editorAction.set(MouseAction::PasteMapBuffer(&this->mapCopyBuffer));
    currentMapView()->requestDraw();
}

void AppDataModel::cut()
{
    MapView *mapView = currentMapView();
    if (mapView)
    {
        mapCopyBuffer.copySelection(*mapView);
        mapView->deleteSelectedItems();
    }
}

void AppDataModel::setCursorShape(int shape)
{
    QGuiApplication::setOverrideCursor(static_cast<Qt::CursorShape>(shape));
}

void AppDataModel::resetCursorShape()
{
    QGuiApplication::restoreOverrideCursor();
}

void AppDataModel::saveMap()
{
    MapView *mapView = currentMapView();
    if (mapView)
    {
        const Map *map = mapView->map();
        if (map)
        {
            if (mapView->map()->filePath().has_value())
            {
                SaveMap::saveMap(*map);
            }
            else
            {
                emit openFolderDialog(QVariant::fromValue(QString("save_map_location")));
            }
        }
    }
}

void AppDataModel::saveCurrentMap(QUrl url)
{
    MapView *mapView = currentMapView();
    if (mapView)
    {
        const Map *map = mapView->map();
        if (map)
        {
            auto filepath = url.toLocalFile();
            mapView->setMapFilepath(std::filesystem::path(filepath.toStdString()));
            bool ok = SaveMap::saveMap(*map);
            if (!ok)
            {
                VME_LOG_ERROR("Failed to save current map to " << filepath.toStdString());
            }
        }
    }
}

void AppDataModel::loadMap(QUrl url)
{
    auto filepath = url.toLocalFile();
    _stagedMap = std::make_unique<Map>(LoadMap::loadMap(std::filesystem::path(filepath.toStdString())));
    addMapTab(url.fileName().toStdString());
}

void AppDataModel::mapTabCreated(QmlMapItem *item, int id)
{
    VME_LOG_D("AppDataModel::mapTabCreated: " << id);
    auto mapTab = QmlMapItemStore::qmlMapItemStore.mapTabs()->getById(id);
    if (mapTab)
    {
        item->setEntryId(id);
        mapTab->item = item;

        if (_stagedMap)
        {
            mapTab->item->setMap(std::move(_stagedMap));
            _stagedMap.reset();

            int index = QmlMapItemStore::qmlMapItemStore.mapTabs()->size() - 1;
            setcurrentMapIndex(index);
        }
    }
}

void AppDataModel::closeMap(int id)
{
    auto mapTab = QmlMapItemStore::qmlMapItemStore.mapTabs()->getById(id);
    if (mapTab)
    {
        // TODO Check if the map has been changed and prompt to save

        QmlMapItemStore::qmlMapItemStore.mapTabs()->removeTabById(id);
    }
}

TownListModel *AppDataModel::townListModel()
{
    return _townListModel.get();
}

FilteredSearchModel *AppDataModel::filteredSearchModel()
{
    return _filteredSearchModel.get();
}

void AppDataModel::toggleAutoBorder()
{
    Settings::AUTO_BORDER = !Settings::AUTO_BORDER;
    emit autoBorderChanged(Settings::AUTO_BORDER);
}

void AppDataModel::toggleShowAnimation()
{
    Settings::RENDER_ANIMATIONS = !Settings::RENDER_ANIMATIONS;

    auto mapView = currentMapView();
    if (mapView && Settings::RENDER_ANIMATIONS)
    {
        mapView->requestDraw();
    }

    emit showAnimationChanged(Settings::RENDER_ANIMATIONS);
}

void AppDataModel::toggleDetailedBorderModeEnabled()
{
    auto mapView = currentMapView();
    if (mapView)
    {
        auto action = mapView->editorAction.as<MouseAction::MapBrush>();
        if (action)
        {
            if (action->brush->type() == BrushType::Border)
            {
                auto brushType = Settings::BORDER_BRUSH_VARIATION == BorderBrushVariationType::General
                                     ? BorderBrushVariationType::Detailed
                                     : BorderBrushVariationType::General;

                BorderBrush::setBrushVariation(brushType);
                emit detailedBorderModeEnabledChanged(brushType == BorderBrushVariationType::Detailed);
                mapView->requestDraw();
            }
            else if (action->brush->type() == BrushType::Mountain)
            {
                Settings::PLACE_MOUNTAIN_FEATURES = !Settings::PLACE_MOUNTAIN_FEATURES;
            }
        }
    }
}

bool AppDataModel::autoBorder() const
{
    return Settings::AUTO_BORDER;
}

bool AppDataModel::showAnimation() const
{
    return Settings::RENDER_ANIMATIONS;
}

bool AppDataModel::detailedBorderModeEnabled() const
{
    return Settings::BORDER_BRUSH_VARIATION == BorderBrushVariationType::Detailed;
}

void AppDataModel::changeBrushInsertionOffset(int delta)
{
    int offset = Settings::BRUSH_INSERTION_OFFSET;
    auto mapView = currentMapView();

    /**
     * If the hovered tile is not null, we can use the item count to determine the maximum offset.
     * This should improve the UX of this functionality.
     */
    if (mapView)
    {
        auto tile = mapView->hoveredTile();
        if (tile)
        {
            auto count = static_cast<int>(tile->itemCount());
            offset = std::min(offset, count);
        }
    }

    Settings::BRUSH_INSERTION_OFFSET = std::max(offset + delta, 0);
    VME_LOG_D("Settings::BRUSH_INSERTION_OFFSET: " << Settings::BRUSH_INSERTION_OFFSET);
}

void AppDataModel::resetBrushInsertionOffset()
{
    Settings::BRUSH_INSERTION_OFFSET = 0;
}

void AppDataModel::createTown()
{
    auto mapView = currentMapView();
    const Town &town = mapView->mapAsShared()->addTown();
    _townListModel->addTown(town);
}

void AppDataModel::qmlSearchEvent(QString searchTerm)
{
    search(searchTerm.toStdString());
}

void AppDataModel::search(std::string searchTerm)
{
    VME_LOG_D("Search for term: " << searchTerm);
    if (searchTerm.size() > 2)
    {
        auto results = Brush::search(searchTerm);
        searchResultModel.setSearchResults(std::move(results));
    }
    else
    {
        searchResultModel.clear();
    }
}

void AppDataModel::onSearchBrushSelected(int index)
{
    auto modelIndex = _filteredSearchModel->index(index, 0);
    auto indexInUnFilteredList = _filteredSearchModel->data(modelIndex, to_underlying(SearchResultModel::Role::VectorIndex)).toInt();

    this->selectBrush(searchResultModel.brushAtIndex(indexInUnFilteredList), true);
}

#include "moc_app_data_model.cpp"