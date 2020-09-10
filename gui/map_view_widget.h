#pragma once

#include <QAbstractScrollArea>

#include "vulkan_window.h"

class QWidget;
class QSize;
class QResizeEvent;

class QtMapViewWidget : public QAbstractScrollArea
{
public:
  QtMapViewWidget(VulkanWindow *window, QWidget *parent = nullptr);

  void scrollContentsBy(int dx, int dy) override;
  QSize viewportSizeHint() const override;
  QSize sizeHint() const override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void updateScrollBars();

  VulkanWindow *vulkanWindow;
};