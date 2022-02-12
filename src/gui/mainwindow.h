#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

#include <QStackedLayout>
#include <QString>
#include <QWidget>
#include <QWidgetAction>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLayout;
class QLabel;
class QTabWidget;
class QPushButton;
class BorderLayout;
class QWindow;
class QVBoxLayout;
class QMenuBar;
class QListView;
QT_END_NAMESPACE

class MapTabWidget;
class ItemPropertyWindow;
class SearchPopupView;
class ItemPaletteWindow;
class TilesetListView;
class SearchPopupWidget;

#include "../map_copy_buffer.h"
#include "../signal.h"
#include "../time_util.h"
#include "gui.h"
#include "minimap.h"
#include "qt_util.h"
#include "vulkan_window.h"

#define QT_MANAGED_POINTER(cls, ...) new cls(__VA_ARGS__);

class MainWindow : public QWidget
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);

    void setVulkanInstance(QVulkanInstance *instance);
    void initializeUI();

    void addMapTab();

    /**
     * @throws MapLoadError if the map could not be loaded.
     */
    void addMapTab(std::shared_ptr<Map> map);

    /**
     * @brief 
     * 
     * @throws MapLoadError if the map could not be loaded.
     */
    void addMapTab(const std::filesystem::path &path);

    bool vulkanWindowEvent(QEvent *event);

    MapView *currentMapView() const noexcept;
    VulkanWindow *currentVulkanWindow() const noexcept;

    bool selectBrush(Brush *brush) noexcept;

    void setSearchVisible(bool visible);
    void setPropertyPanelVisible(bool visible);

    void windowPressEvent(QWindow *window, QMouseEvent *event);

    void requestMinimapUpdate();

    EditorAction editorAction;

    MapCopyBuffer &getMapCopyBuffer();

    bool hasCopyBuffer() const;

    void copySelection();

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;

  private:
    uint32_t nextUntitledId();

    void keyPressEvent(QKeyEvent *event) override;

    void mapViewMousePosEvent(MapView &mapView, util::Point<float> mousePos);
    void mapViewSelectionChangedEvent(MapView &mapView);
    void mapViewSelectedTileThingClicked(MapView *mapView, const Tile *tile, TileThing tileThing);
    void mapViewUndoRedoEvent(MapView &mapView);
    void mapViewViewportEvent(MapView &mapView, const Camera::Viewport &viewport);
    void mapTabCloseEvent(int index, QVariant data);
    void mapTabChangedEvent(int index);

    void editorActionChangedEvent(const MouseAction_t &action);

    QMenuBar *createMenuBar();
    void initializePaletteWindow();

    void registerPropertyItemListeners();

    MapTabWidget *mapTabs = nullptr;
    ItemPropertyWindow *propertyWindow = nullptr;
    QWidget *propertyWindowContainer = nullptr;
    ItemPaletteWindow *_paletteWindow = nullptr;
    MinimapWidget *_minimapWidget = nullptr;

    // BorderLayout *rootLayout = nullptr;
    QVBoxLayout *rootLayout = nullptr;

    QLabel *positionStatus = nullptr;
    QLabel *zoomStatus = nullptr;
    QLabel *topItemInfo = nullptr;

    uint32_t highestUntitledId = 0;
    std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> untitledIds;

    QVulkanInstance *vulkanInstance = nullptr;

    MapCopyBuffer mapCopyBuffer;

    SearchPopupWidget *searchPopupWidget = nullptr;

    TimePoint lastUiToggleTime;
};

class MainLayout : public QStackedLayout
{
  public:
    MainLayout(QWidget *mainWidget);

    void setSearchWidget(SearchPopupWidget *searchWidget);
    void showSearch();
    void hideSearch();

    bool isSearchVisible() const noexcept;

  private:
    QWidget *mainWidget;
    SearchPopupWidget *searchWidget;

    bool searchVisible = false;
};