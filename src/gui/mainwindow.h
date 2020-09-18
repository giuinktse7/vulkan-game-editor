#pragma once

#include <memory>
#include <queue>
#include <vector>
#include <functional>

#include <QWidget>
#include <QWidgetAction>
#include <QString>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QLayout;
class QLabel;
class QTabWidget;
class QPushButton;
class BorderLayout;
QT_END_NAMESPACE

class MapTabWidget;

#include "vulkan_window.h"

#define QT_MANAGED_POINTER(cls, ...) new cls(__VA_ARGS__);

class MainWindow : public QWidget
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);

  void setVulkanInstance(QVulkanInstance *instance);

  void addMapTab();
  void addMapTab(std::shared_ptr<Map> map);

  MapTabWidget *mapTabs;

protected:
  void mousePressEvent(QMouseEvent *event) override;

private:
  // UI
  BorderLayout *rootLayout;

  QLabel *positionStatus;

  uint32_t highestUntitledId = 0;
  std::priority_queue<uint32_t, std::vector<uint32_t>, std::greater<uint32_t>> untitledIds;

  uint32_t nextUntitledId();

  void experimentLayout();
  void initializeUI();

  QMenuBar *createMenuBar();
  void createMapTabArea();

  void mapViewMousePosEvent(MapView &mapView, util::Point<float> mousePos);
  void mapViewViewportEvent(MapView &mapView, const Viewport &viewport);

  QVulkanInstance *vulkanInstance;

  // void updatePositionText();
};
