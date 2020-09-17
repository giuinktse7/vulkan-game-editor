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
constexpr int DefaultSingleStepSize = MapTileSize;

constexpr float MinCameraZoom = 0.1f;

constexpr std::pair<float, float> singleStepKM()
{
  int t = DefaultSingleStepSize;
  int a = MinCameraZoom;
  int s = MinimumStepSizeInPixels;

  float m = (a * t - s) / (a - 1);
  float k = t - m;

  return {k, m};
}
constexpr std::pair<float, float> km = singleStepKM();

int computeSingleStep(float zoom)
{
  const auto [k, m] = km;
  double step = k * zoom + m;
  // VME_LOG_D("\n\nNext:");

  // VME_LOG_D("zoom: " << zoom);
  // VME_LOG_D("step: " << step);

  // Scale zoomed out step
  if (zoom > 1)
  {
    const double k = 0.2;
    double scaledStep = step * (3 * (zoom - 1) + (std::exp(k * zoom) - std::exp(k) + 1));

    // VME_LOG_D("scaledStep: " << scaledStep);
    return static_cast<int>(std::round(scaledStep));
  }
  else
  {
    return static_cast<int>(std::round(step));
  }
}

void QtMapViewWidget::viewportChanged(const Viewport &viewport)
{
  hbar->setValue(viewport.offset.x);
  vbar->setValue(viewport.offset.y);

  hbar->setPageStep(viewport.width);
  vbar->setPageStep(viewport.height);

  int singleStep = computeSingleStep(viewport.zoom);
  // VME_LOG_D("singleStep: " << singleStep);
  hbar->setSingleStep(singleStep);
  vbar->setSingleStep(singleStep);

  emit viewportChangedEvent(viewport);
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

  connect(window, &VulkanWindow::keyPressedEvent, this, &QtMapViewWidget::onWindowKeyPress);

  // viewport()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  resize(500, 500);
}

void QtMapViewWidget::onWindowKeyPress(QKeyEvent *event)
{
  switch (event->key())
  {
  case Qt::Key_Left:
    hbar->setValue(hbar->value() - hbar->singleStep());
    break;
  case Qt::Key_Right:
    hbar->setValue(hbar->value() + hbar->singleStep());
    break;
  case Qt::Key_Up:
    vbar->setValue(vbar->value() - vbar->singleStep());
    break;
  case Qt::Key_Down:
    vbar->setValue(vbar->value() + vbar->singleStep());
    break;
  default:
    event->ignore();
    break;
  }
}

// bool QtMapViewWidget::event(QEvent *event)
// {
//   qDebug() << this << ": " << event;
//   if (event->type() == QEvent::Hide)
//   {
//     if (this->windowHandle()->isVisible())
//     {
//       this->windowHandle()->hide();
//     }
//     // TODO Maybe remove
//     setUpdatesEnabled(false);
//   }

//   event->ignore();
//   return QAbstractScrollArea::event(event);
// }

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
  // event->ignore();
  // QAbstractScrollArea::keyPressEvent(event);
  VME_LOG_D("QtMapViewWidget::keyPressEvent");
}

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

  /*
    After changing the scroll bar position, process the event again. Now, the scroll bar is below the mouse.
    This lets the user drag the scroll bar directly without having to press again.
  */
  e->ignore();
  mousePressEvent(e);
}

void QtScrollBar::mouseMoveEvent(QMouseEvent *e)
{
  QScrollBar::mouseMoveEvent(e);
}
