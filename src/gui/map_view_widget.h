#pragma once

#include <QAbstractScrollArea>
#include <QScrollBar>

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
};

class QtMapViewWidget : public QAbstractScrollArea, public MapView::Observer
{
  Q_OBJECT
public:
  QtMapViewWidget(VulkanWindow *window, QWidget *parent = nullptr);

  void scrollContentsBy(int dx, int dy) override;
  // QSize viewportSizeHint() const override;
  // QSize sizeHint() const override;
  // void resizeEvent(QResizeEvent *event) override;

  // From MapView::Observer
  void viewportChanged(const Viewport &viewport) override;

signals:
  void viewportChangedEvent(const Viewport &viewport);

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  VulkanWindow *vulkanWindow;
  MapView *mapView;

  QtScrollBar *hbar;
  QtScrollBar *vbar;

  void onWindowKeyPress(QKeyEvent *event);
};
