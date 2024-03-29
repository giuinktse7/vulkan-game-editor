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
#include "search_result_model.h"
#include "town_list_model.h"

#include "combobox_model.h"

class TileSetModel;
class ContextMenuModel;
class TownListModel;
class FilteredSearchModel;
class QmlMapItem;
class ComboBoxModel;

class ItemPaletteViewData
{
  public:
    ItemPaletteViewData(ComboBoxModel *paletteDropdownModel,
                        ComboBoxModel *tilesetDropdownModel,
                        QObject *brushList,
                        TileSetModel *tilesetModel);

    void positionViewAtIndex(int index) const;

    ComboBoxModel *paletteDropdownModel;
    ComboBoxModel *tilesetDropdownModel;
    TileSetModel *tilesetModel;
    QObject *brushList;
};

class AppDataModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MapTabListModel *mapTabs READ mapTabs)
    Q_PROPERTY(TownListModel *townListModel READ townListModel)
    Q_PROPERTY(FilteredSearchModel *filteredSearchModel READ filteredSearchModel)
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

    Q_INVOKABLE void initializeItemPalettePanel(ComboBoxModel *paletteDropdownModel, ComboBoxModel *tilesetDropdownModel, QObject *brushList, TileSetModel *tilesetModel);
    Q_INVOKABLE void destroyItemPalettePanel(ComboBoxModel *paletteModel);
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
    Q_INVOKABLE void createTown();

    Q_INVOKABLE void qmlSearchEvent(QString searchTerm);
    Q_INVOKABLE void onSearchBrushSelected(int index);

    void addMapTab(std::string tabName);
    void removeMapTab(int index);

    MapView *currentMapView();
    std::weak_ptr<MapView> currentMapViewPtr();

    // Actions
    void selectBrush(Brush *brush, bool showInItemPalette = false);

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

    TownListModel *townListModel();
    FilteredSearchModel *filteredSearchModel();

    void setcurrentMapIndex(int index);

    void search(std::string searchTerm);

  signals:
    void currentMapViewChanged(MapView *prev, MapView *current);
    void currentMapIndexChanged(int index);
    void autoBorderChanged(bool value);
    void showAnimationChanged(bool value);
    void detailedBorderModeEnabledChanged(bool value);

  private:
    int _currentMapIndex = -1;

    /**
     * A map that has been loaded but not yet added to a QmlMapItem.
     */
    std::unique_ptr<Map> _stagedMap;

    MapCopyBuffer mapCopyBuffer;

    std::optional<Position> _contextMenuPosition;

    SearchResultModel searchResultModel;

    std::vector<ItemPaletteViewData> itemPalettes;

  private:
    std::unique_ptr<TownListModel> _townListModel;
    std::unique_ptr<FilteredSearchModel> _filteredSearchModel;
};
