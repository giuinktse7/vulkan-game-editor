
#include "editor.h"

#include "core/logger.h"

Editor::Editor()
{
    addMapTab("Untitled-1");
    addMapTab("Untitled-2");
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

void Editor::applyBrush(Brush *brush)
{
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
