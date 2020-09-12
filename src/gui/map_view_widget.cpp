#include "map_view_widget.h"

#include <QSize>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStyle>
#include <QStyleOptionSlider>

#include <QApplication>
#include <QWidget>
#include <QDesktopWidget>
#include <QScreen>

#include "../const.h"
#include "../map.h"
#include "../logger.h"

#include "../qt/logging.h"

constexpr int MinimumStepSizeInPixels = MapTileSize / 3;

void QtMapViewWidget::viewportChanged(const Viewport &viewport)
{
  hbar->setValue(viewport.offset.x);
  vbar->setValue(viewport.offset.y);

  hbar->setPageStep(viewport.width);
  vbar->setPageStep(viewport.height);
}

QtMapViewWidget::QtMapViewWidget(VulkanWindow *window, QWidget *parent)
    : MapView::Observer(window->getMapView()),
      QAbstractScrollArea(parent),
      vulkanWindow(window),
      mapView(window->getMapView()),
      hbar(new QtScrollBar(Qt::Horizontal, this)),
      vbar(new QtScrollBar(Qt::Vertical, this))
{
  setHorizontalScrollBar(hbar);
  setVerticalScrollBar(vbar);

  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setViewport(window->wrapInWidget());

  Map *map = mapView->getMap();
  uint16_t width = map->getWidth();
  uint16_t height = map->getHeight();

  int maxX = width * MapTileSize / MinimumStepSizeInPixels;
  int maxY = height * MapTileSize / MinimumStepSizeInPixels;

  hbar->setMinimum(0);
  hbar->setMaximum(maxX);
  hbar->setSingleStep(MapTileSize);

  vbar->setMinimum(0);
  vbar->setMaximum(maxY);
  vbar->setSingleStep(MapTileSize);

  // viewport()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  resize(500, 500);
}

// QSize QtMapViewWidget::sizeHint() const
// {
//   QSize s = viewport()->sizeHint();

//   s.setWidth(s.width() + vbar->sizeHint().width());
//   s.setHeight(s.height() + hbar->sizeHint().height());

//   return s;
// }

// QSize QtMapViewWidget::viewportSizeHint() const
// {
//   return viewport()->sizeHint();
// }

/*
  dx and dy are calculated as (from - to).
  Scrolling downwards or to the right results in negative dx and dy.
*/
void QtMapViewWidget::scrollContentsBy(int dx, int dy)
{
  // VME_LOG_D("Scroll by" << dx << ", " << dy);
  auto viewportX = hbar->value();
  auto viewportY = vbar->value();

  vulkanWindow->getMapView()->setCameraPosition(WorldPosition(viewportX, viewportY));
}

/**
 * ************************
 * Events
 * ************************
 * ************************
 */
// void QtMapViewWidget::resizeEvent(QResizeEvent *event)
// {
// horizontalScrollBar()->update();
// verticalScrollBar()->update();
// viewport()->update();
// }

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

// void QtMapViewWidget::keyPressEvent(QKeyEvent *event)
// {
//   event->ignore();
//   QAbstractScrollArea::keyPressEvent(event);
//   VME_LOG_D("QtMapViewWidget::keyPressEvent");
// }

void QtMapViewWidget::wheelEvent(QWheelEvent *event)
{
  VME_LOG_D("QtMapViewWidget::wheelEvent");
}

QtScrollBar::QtScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QScrollBar(orientation, parent)
{
}

void QtScrollBar::mousePressEvent(QMouseEvent *e)
{
  QStyleOptionSlider opt;
  initStyleOption(&opt);
  QStyle::SubControl subControl = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, e->pos(), this);
  bool scrollbarJump = subControl == QStyle::SC_ScrollBarAddPage || subControl == QStyle::SC_ScrollBarSubPage;

  if (!scrollbarJump)
  {
    QScrollBar::mousePressEvent(e);
    return;
  }

  QRect grooveRect = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                             QStyle::SC_ScrollBarGroove, this);
  QRect sliderRect = style()->subControlRect(QStyle::CC_ScrollBar, &opt,
                                             QStyle::SC_ScrollBarSlider, this);

  int sliderMin, sliderMax, sliderLength, pos;
  if (orientation() == Qt::Horizontal)
  {
    pos = e->pos().x();
    sliderLength = sliderRect.width();
    sliderMin = grooveRect.x();
    sliderMax = grooveRect.right() - sliderLength + 1;
    if (layoutDirection() == Qt::RightToLeft)
      opt.upsideDown = !opt.upsideDown;
  }
  else // Qt::Vertical
  {
    pos = e->pos().y();
    sliderLength = sliderRect.height();
    sliderMin = grooveRect.y();
    sliderMax = grooveRect.bottom() - sliderLength + 1;
  }

  // Place the new scrollbar in the center of the click
  pos -= sliderLength / 2;

  if (pos < sliderMin)
  {
    setValue(minimum());
  }
  else if (pos > sliderMax)
  {
    setValue(maximum());
  }
  else
  {
    int value = QStyle::sliderValueFromPosition(minimum(), maximum(), pos - sliderMin,
                                                sliderMax - sliderMin, opt.upsideDown);
    setValue(value);
  }
  e->ignore();
  mousePressEvent(e);
}

void QtScrollBar::mouseMoveEvent(QMouseEvent *e)
{
  QScrollBar::mouseMoveEvent(e);
}
