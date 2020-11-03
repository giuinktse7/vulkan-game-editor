#pragma once

#include <QAbstractScrollArea>
#include <QScrollBar>
#include <QWidget>

#include "vulkan_window.h"

#include "../map_view.h"
#include "../signal.h"

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

class MapViewWidget : public QWidget, public Nano::Observer<>
{
  Q_OBJECT
public:
  MapViewWidget(VulkanWindow *window, QWidget *parent = nullptr);

  void viewportChanged(const Camera::Viewport &viewport);
  void mapViewDrawRequested();
  void selectionChanged();

  MapView *mapView;

signals:
  void viewportChangedEvent(const Camera::Viewport &viewport);
  void selectionChangedEvent();

private:
  VulkanWindow *vulkanWindow;

  QtScrollBar *hbar;
  QtScrollBar *vbar;

  void onWindowKeyPress(QKeyEvent *event);
  void setMapViewX(int value);
  void setMapViewY(int value);
};
