#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <memory>
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
    Q_PROPERTY(bool autoBorder READ autoBorder NOTIFY autoBorderChanged)
    Q_PROPERTY(bool showAnimation READ showAnimation NOTIFY showAnimationChanged)
    Q_PROPERTY(bool detailedBorderModeEnabled READ detailedBorderModeEnabled NOTIFY detailedBorderModeEnabledChanged)

  signals:
    void openFolderDialog(QVariant dialog_id);

  public:
    AppDataModel();

    Q_INVOKABLE void mapTabSelected(int prevIndex, int index);
    Q_INVOKABLE void loadMap(QUrl url);
    Q_INVOKABLE void saveCurrentMap(QUrl url);

    Q_INVOKABLE void setItemPaletteList(ComboBoxModel *model);
    Q_INVOKABLE void setItemPalette(ComboBoxModel *model, QString paletteId);
    Q_INVOKABLE void setTileset(TileSetModel *model, QString paletteId, QString tilesetId);
    Q_INVOKABLE void selectBrush(TileSetModel *model, int index);
    Q_INVOKABLE void populateRightClickContextMenu(ContextMenuModel *model);
    Q_INVOKABLE void onContextMenuAction(int id);

    Q_INVOKABLE void copy();
    Q_INVOKABLE void paste();
    Q_INVOKABLE void cut();
    Q_INVOKABLE void setCursorShape(int shape);
    Q_INVOKABLE void resetCursorShape();
    Q_INVOKABLE void saveMap();
    Q_INVOKABLE void closeMap(int id);

    Q_INVOKABLE void mapTabCreated(QmlMapItem *item, int id);

    Q_INVOKABLE void changeBrushInsertionOffset(int delta);
    Q_INVOKABLE void resetBrushInsertionOffset();

    Q_INVOKABLE void toggleAutoBorder();
    Q_INVOKABLE void toggleShowAnimation();
    Q_INVOKABLE void toggleDetailedBorderModeEnabled();

    void addMapTab(std::string tabName);
    void removeMapTab(int index);

    MapView *currentMapView();

    // Actions
    void selectBrush(Brush *brush);

    int currentMapIndex() const
    {
        return _currentMapIndex;
    }

    bool autoBorder() const;
    bool showAnimation() const;
    bool detailedBorderModeEnabled() const;

    MapTabListModel *mapTabs()
    {
        return QmlMapItemStore::qmlMapItemStore.mapTabs();
    };

    void setcurrentMapIndex(int index);

  signals:
    void currentMapIndexChanged(int index);
    void autoBorderChanged(bool value);
    void showAnimationChanged(bool value);
    void detailedBorderModeEnabledChanged(bool value);

  private:
    int _currentMapIndex = 0;

    /**
     * A map that has been loaded but not yet added to a QmlMapItem.
     */
    std::unique_ptr<Map> _stagedMap;

    MapCopyBuffer mapCopyBuffer;

    std::optional<Position> _contextMenuPosition;
};
