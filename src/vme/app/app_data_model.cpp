
#include "app_data_model.h"

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

#include <memory>
#include <ranges>

AppDataModel::AppDataModel()
{
    addMapTab("Untitled-1");
}

void AppDataModel::mapTabSelected(int prevIndex, int index)
{
    VME_LOG_D("mapTabSelected from C++: " << prevIndex << ", " << index);

    mapTabs()->get(prevIndex).item->setActive(false);
    mapTabs()->get(index).item->setActive(true);
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

void AppDataModel::setItemPaletteList(ComboBoxModel *model)
{
    model->setData(ItemPalettes::getItemPaletteList());
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
        VME_LOG_D("Set data to palette: " << paletteId.toStdString());
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
    VME_LOG_D("populateRightClickContextMenu from C++: ");

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

void AppDataModel::selectBrush(Brush *brush)
{
    VME_LOG_D("AppDataModel::selectBrush");
    auto mapView = currentMapView();

    EditorAction::editorAction.setBrush(brush);
    mapView->requestDraw();
}

void AppDataModel::setcurrentMapIndex(int index)
{
    if (_currentMapIndex != index)
    {
        _currentMapIndex = index;
        emit currentMapIndexChanged(index);

        auto mapView = currentMapView();
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

#include "moc_app_data_model.cpp"