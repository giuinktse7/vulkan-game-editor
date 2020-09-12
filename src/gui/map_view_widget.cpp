#include "map_view_widget.h"

#include <QScrollBar>
#include <QSize>

#include "../const.h"
#include "../map.h"
#include "../logger.h"

constexpr int MinimumStepSizeInPixels = MapTileSize / 3;

QtMapViewWidget::QtMapViewWidget(VulkanWindow *window, QWidget *parent)
    : QAbstractScrollArea(parent), vulkanWindow(window)
{
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setViewport(window->wrapInWidget());

  Map *map = window->getMapView()->getMap();
  uint16_t width = map->getWidth();
  uint16_t height = map->getHeight();

  int maxX = width * MapTileSize / MinimumStepSizeInPixels;
  int maxY = height * MapTileSize / MinimumStepSizeInPixels;

  horizontalScrollBar()->setMinimum(0);
  horizontalScrollBar()->setMaximum(maxX);

  // horizontalScrollBar()->setSingleStep(MinimumStepSizeInPixels *);

  verticalScrollBar()->setMinimum(0);
  verticalScrollBar()->setMaximum(maxY);

  connect(vulkanWindow, &VulkanWindow::panEvent, this, [=](int dx, int dy) { this->pan(dx, dy); });

  connect(horizontalScrollBar(), &QScrollBar::valueChanged, [=](int x) { this->vulkanWindow->getMapView()->setX(x); });
  connect(verticalScrollBar(), &QScrollBar::valueChanged, [=](int y) { this->vulkanWindow->getMapView()->setY(y); });

  // horizontalScrollBar()->setValue(horizontalScrollBar()->maximum() / 2);
  // verticalScrollBar()->setValue(verticalScrollBar()->maximum() / 2);

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

void QtMapViewWidget::pan(int dx, int dy)
{
  VME_LOG_D("QtMapViewWidget::pan");
  horizontalScrollBar()->setValue(horizontalScrollBar()->value() + dx);
  verticalScrollBar()->setValue(verticalScrollBar()->value() + dy);
}

/*
  dx and dy are calculated as (from - to).
  Scrolling downwards or to the right results in negative dx and dy.
*/
void QtMapViewWidget::scrollContentsBy(int dx, int dy)
{
  auto viewportX = horizontalScrollBar()->value();
  auto viewportY = verticalScrollBar()->value();

  vulkanWindow->getMapView()->setCameraPosition(WorldPosition(viewportX, viewportY));
}

/**
 * ************************
 * Events
 * ************************
 * ************************
 */
void QtMapViewWidget::resizeEvent(QResizeEvent *event)
{
  // TODO
  horizontalScrollBar()->update();
  verticalScrollBar()->update();
  viewport()->update();
}

void QtMapViewWidget::mousePressEvent(QMouseEvent *event)
{
  VME_LOG_D("QtMapViewWidget::mousePressEvent");
}

void QtMapViewWidget::mouseReleaseEvent(QMouseEvent *event)
{
  VME_LOG_D("QtMapViewWidget::mouseReleaseEvent");
}

void QtMapViewWidget::mouseMoveEvent(QMouseEvent *event)
{
  VME_LOG_D("QtMapViewWidget::mouseMoveEvent");
}

void QtMapViewWidget::keyPressEvent(QKeyEvent *event)
{
  VME_LOG_D("QtMapViewWidget::keyPressEvent");
}

void QtMapViewWidget::wheelEvent(QWheelEvent *event)
{
  VME_LOG_D("QtMapViewWidget::wheelEvent");
}