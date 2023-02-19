
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

    // TODO Abstract the tileset concept and use data files to build palettes & tilesets
    _itemPaletteStore.setTileset(defaultTileset());

    _itemPaletteStore.onSelectBrush<&Editor::selectBrush>(this);
}

void Editor::mapTabSelected(int prevIndex, int index)
{
    VME_LOG_D("mapTabSelected from C++: " << prevIndex << ", " << index);

    mapTabs()->get(prevIndex).item->setActive(false);
    mapTabs()->get(index).item->setActive(true);
}

void Editor::addMapTab(std::string tabName)
{
    mapTabs()->addTab(tabName);
}

void Editor::removeMapTab(int index)
{
    mapTabs()->removeTab(index);
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

void Editor::setcurrentMapIndex(int index)
{
    if (_currentMapIndex != index)
    {
        _currentMapIndex = index;
        emit currentMapIndexChanged(index);

        auto mapView = currentMapView();
    }
}
