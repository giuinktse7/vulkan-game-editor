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
class QListView;
QT_END_NAMESPACE

class MapTabWidget;
class ItemPropertyWindow;

#include "gui.h"
#include "signal.h"
#include "vulkan_window.h"

#define QT_MANAGED_POINTER(cls, ...) new cls(__VA_ARGS__);

class MainWindow : public QWidget, public Nano::Observer<>
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

  void setVulkanInstance(QVulkanInstance *instance);

  void addMapTab();
  void addMapTab(std::shared_ptr<Map> map);

protected:
  void mousePressEvent(QMouseEvent *event) override;

private:
  EditorAction editorAction;

  // UI
  MapTabWidget *mapTabs;
  ItemPropertyWindow *propertyWindow;

  BorderLayout *rootLayout;

  QLabel *positionStatus;
  QLabel *zoomStatus;

  uint32_t highestUntitledId = 0;
  std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> untitledIds;

  QVulkanInstance *vulkanInstance;

  MapView *currentMapView() const noexcept;

  uint32_t nextUntitledId();

  void keyPressEvent(QKeyEvent *event) override;

  void mapViewMousePosEvent(MapView &mapView, util::Point<float> mousePos);
  void mapViewViewportEvent(MapView &mapView, const Camera::Viewport &viewport);
  void mapViewSelectionChangedEvent();
  void mapTabCloseEvent(int index, QVariant data);
  void mapTabChangedEvent(int index);

  void editorActionChangedEvent(const MouseAction_t &action);

  void initializeUI();
  QMenuBar *createMenuBar();
  QListView *createItemPalette();

  // void updatePositionText();
};

class ItemListEventFilter : public QObject
{
public:
  ItemListEventFilter(QObject *parent = nullptr) : QObject(parent) {}

protected:
  bool eventFilter(QObject *obj, QEvent *event) override;
};