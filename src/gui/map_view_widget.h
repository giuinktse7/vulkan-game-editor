#pragma once

#include <QAbstractScrollArea>

#include "vulkan_window.h"

class QWidget;
class QSize;
class QResizeEvent;
class QMouseEvent;
class QKeyEvent;
class QWheelEvent;

class QtMapViewWidget : public QAbstractScrollArea
{
  Q_OBJECT
public:
  QtMapViewWidget(VulkanWindow *window, QWidget *parent = nullptr);

  void scrollContentsBy(int dx, int dy) override;
  QSize viewportSizeHint() const override;
  QSize sizeHint() const override;
  void resizeEvent(QResizeEvent *event) override;

public slots:
  void pan(int dx, int dy);

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  void updateScrollBars();

  VulkanWindow *vulkanWindow;
};