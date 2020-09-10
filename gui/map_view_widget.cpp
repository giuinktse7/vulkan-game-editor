#include "map_view_widget.h"

#include <QScrollBar>
#include <QSize>

#include "../const.h"
#include "../map.h"
#include "../logger.h"

QtMapViewWidget::QtMapViewWidget(VulkanWindow *window, QWidget *parent)
    : QAbstractScrollArea(parent), vulkanWindow(window)
{
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setViewport(window->wrapInWidget());

  Map *map = window->getMapView()->getMap();
  uint16_t width = map->getWidth();
  uint16_t height = map->getHeight();

  horizontalScrollBar()->setMinimum(0);
  horizontalScrollBar()->setMaximum(width);
  horizontalScrollBar()->setValue(width / 2);

  verticalScrollBar()->setMinimum(0);
  verticalScrollBar()->setMaximum(height);
  verticalScrollBar()->setValue(height / 2);

  // viewport()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  resize(500, 500);
}

QSize QtMapViewWidget::sizeHint() const
{
  QSize s = viewport()->sizeHint();
  auto hbar = horizontalScrollBar();
  auto vbar = verticalScrollBar();

  s.setWidth(s.width() + vbar->sizeHint().width());
  s.setHeight(s.height() + hbar->sizeHint().height());

  return s;
}

QSize QtMapViewWidget::viewportSizeHint() const
{
  return viewport()->sizeHint();
}

void QtMapViewWidget::scrollContentsBy(int dx, int dy)
{
  VME_LOG_D("dx: " << dx << ", dy: " << dy);
  vulkanWindow->getMapView()->translateCamera(WorldPosition(-dx * MapTileSize, -dy * MapTileSize));
}

void QtMapViewWidget::resizeEvent(QResizeEvent *event)
{
  // TODO
  horizontalScrollBar()->update();
  verticalScrollBar()->update();
  viewport()->update();
}
