#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <optional>
#include <string>

#include "core/brushes/brush.h"
#include "core/map_copy_buffer.h"
#include "core/position.h"
#include "qml_map_item.h"

#include "combobox_model.h"

class TileSetModel;
class ContextMenuModel;
class QmlMapItem;

class AppDataModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MapTabListModel *mapTabs READ mapTabs)
    Q_PROPERTY(int currentMapIndex READ currentMapIndex WRITE setcurrentMapIndex NOTIFY currentMapIndexChanged)

  public:
    AppDataModel();

    Q_INVOKABLE void mapTabSelected(int prevIndex, int index);

    Q_INVOKABLE void setItemPaletteList(ComboBoxModel *model);
    Q_INVOKABLE void setItemPalette(ComboBoxModel *model, QString paletteId);
    Q_INVOKABLE void setTileset(TileSetModel *model, QString paletteId, QString tilesetId);
    Q_INVOKABLE void selectBrush(TileSetModel *model, int index);
    Q_INVOKABLE void populateRightClickContextMenu(ContextMenuModel *model);
    Q_INVOKABLE void onContextMenuAction(int id);

    Q_INVOKABLE void copy();
    Q_INVOKABLE void paste();
    Q_INVOKABLE void cut();

    void addMapTab(std::string tabName);
    void removeMapTab(int index);

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

    MapCopyBuffer mapCopyBuffer;

    std::optional<Position> _contextMenuPosition;
};
