#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <vector>

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
class QMenuBar;
class QListView;
QT_END_NAMESPACE

class MapTabWidget;
class ItemPropertyWindow;
class ItemPaletteWindow;
class TilesetListView;

#include "../signal.h"
#include "gui.h"
#include "qt_util.h"
#include "vulkan_window.h"

#define QT_MANAGED_POINTER(cls, ...) new cls(__VA_ARGS__);

class MainWindow : public QWidget, public Nano::Observer<>
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);

    void setVulkanInstance(QVulkanInstance *instance);
    void initializeUI();

    void addMapTab();
    void addMapTab(std::shared_ptr<Map> map);

    bool vulkanWindowEvent(QEvent *event);

    MapView *currentMapView() const noexcept;

    bool selectBrush(Brush *brush) noexcept;

    EditorAction editorAction;

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override;

  private:
    uint32_t nextUntitledId();

    void keyPressEvent(QKeyEvent *event) override;

    void mapViewMousePosEvent(MapView &mapView, util::Point<float> mousePos);
    void mapViewSelectionChangedEvent(MapView &mapView);
    void mapViewUndoRedoEvent(MapView &mapView);
    void mapViewViewportEvent(MapView &mapView, const Camera::Viewport &viewport);
    void mapTabCloseEvent(int index, QVariant data);
    void mapTabChangedEvent(int index);

    void editorActionChangedEvent(const MouseAction_t &action);

    QMenuBar *createMenuBar();
    void initializePaletteWindow();

    MapTabWidget *mapTabs;
    ItemPropertyWindow *propertyWindow;
    ItemPaletteWindow *_paletteWindow;

    BorderLayout *rootLayout;

    QLabel *positionStatus;
    QLabel *zoomStatus;
    QLabel *topItemInfo;

    uint32_t highestUntitledId = 0;
    std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> untitledIds;

    QVulkanInstance *vulkanInstance;

    // void updatePositionText();
};
