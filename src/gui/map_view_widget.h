#pragma once

#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QWidget>

#include "vulkan_window.h"

#include "../map_view.h"

class QWidget;
class QSize;
class QResizeEvent;
class QMouseEvent;
class QKeyEvent;
class QWheelEvent;

class QtScrollBar : public QScrollBar
{
public:
  QtScrollBar(Qt::Orientation orientation, QWidget *parent = nullptr);
  void mousePressEvent(QMouseEvent *e) override;
  void mouseMoveEvent(QMouseEvent *e) override;

  void initStyleOption(QStyleOptionSlider *option) const;

  void addValue(int value);
};

class MapViewWidget : public QWidget, public MapView::Observer
{
  Q_OBJECT
public:
  MapViewWidget(VulkanWindow *window, QWidget *parent = nullptr);

  // From MapView::Observer
  void viewportChanged(const Viewport &viewport) override;

signals:
  void viewportChangedEvent(const Viewport &viewport);

private:
  VulkanWindow *vulkanWindow;
  MapView *mapView;

  QtScrollBar *hbar;
  QtScrollBar *vbar;

  void onWindowKeyPress(QKeyEvent *event);
  void updateMapViewport();
};