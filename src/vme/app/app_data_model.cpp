
#include "app_data_model.h"

#include "core/item_palette.h"
#include "core/logger.h"
#include "core/tileset.h"
#include "tileset_model.h"

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