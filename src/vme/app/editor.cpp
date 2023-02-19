
#include "editor.h"

#include "core/logger.h"
#include "core/tileset.h"
#include "item_palette_store.h"

#include <memory>

std::shared_ptr<Tileset> defaultTileset()
{
    auto tileset = std::make_shared<Tileset>("all", "All");

    int from = 100;
    int to = 45000;

#ifdef _DEBUG_VME
    from = 100;
    to = 5000;
#endif

    for (int i = from; i < to; ++i)
    {
        if (Items::items.validItemType(i))
        {
            tileset->addRawBrush(i);
        }
    }

    return tileset;
}

Editor::Editor()
{
    addMapTab("Untitled-1");
    addMapTab("Untitled-2");

    // TODO Abstract the tileset concept and use data files to build palettes & tilesets
    _itemPaletteStore.setTileset(defaultTileset());

    _itemPaletteStore.onSelectBrush<&Editor::selectBrush>(this);
}

void Editor::mapTabSelected(int prevIndex, int index)
{
    VME_LOG_D("mapTabSelected from C++: " << prevIndex << ", " << index);
}

void Editor::addMapTab(std::string tabName)
{
    mapTabs()->addTab(tabName);
}

void Editor::removeMapTab(int index)
{
    mapTabs()->removeTab(index);
}

void Editor::mapTabCreated(QmlMapItem *item, int index)
{
    mapTabs()->setInstance(index, item);
}

MapView *Editor::currentMapView()
{
    if (mapTabs()->empty())
    {
        return nullptr;
    }

    auto item = mapTabs()->get(_currentMapIndex).item;

    return item ? item->mapView.get() : nullptr;
}

void Editor::selectBrush(Brush *brush)
{
    VME_LOG_D("Editor::selectBrush");
    auto mapView = currentMapView();

    EditorAction::editorAction.setBrush(brush);
    mapView->requestDraw();
}

void Editor::test()
{
    VME_LOG_D("Editor::test");
}

void Editor::setcurrentMapIndex(int index)
{
    if (_currentMapIndex != index)
    {
        _currentMapIndex = index;
        emit currentMapIndexChanged(index);

        auto mapView = currentMapView();
        VME_LOG_D("Current map view: " << currentMapView());
        // TODO Remove, just for testing
        if (mapView)
        {
            mapView->commitTransaction(TransactionType::AddMapItem, [mapView]() {
                mapView->addItem(Position(15, 15, 7), 4526);
            });

            mapView->requestDraw();
        }
    }
}
