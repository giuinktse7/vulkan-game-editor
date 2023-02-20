#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <string>

#include "core/brushes/brush.h"
#include "item_palette_store.h"
#include "qml_map_item.h"

class QmlMapItem;

class Editor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MapTabListModel *mapTabs READ mapTabs)
    Q_PROPERTY(int currentMapIndex READ currentMapIndex WRITE setcurrentMapIndex NOTIFY currentMapIndexChanged)

  public:
    Editor();

    Q_INVOKABLE void mapTabSelected(int prevIndex, int index);

    void addMapTab(std::string tabName);
    void removeMapTab(int index);

    ItemPaletteStore &itemPaletteStore()
    {
        return _itemPaletteStore;
    }

    MapView *currentMapView();

    // Actions
    void selectBrush(Brush *brush);

    int currentMapIndex() const
    {
        return _currentMapIndex;
    }

    MapTabListModel *mapTabs()
    {
        return QmlMapItemStore::qmlMapItemStore.mapTabs();
    };

    void setcurrentMapIndex(int index);

  signals:
    void currentMapIndexChanged(int index);

  private:
    int _currentMapIndex = 0;

    ItemPaletteStore _itemPaletteStore;
};
