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

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>

#include "../const.h"
#include "../map.h"
#include "../logger.h"

#include "../qt/logging.h"
#include "border_layout.h"

constexpr int MinimumStepSizeInPixels = MapTileSize / 3;
constexpr int DefaultSingleStepSize = MapTileSize;

constexpr float MinCameraZoom = 0.1f;

/*
  Calculates the k and m parameters for the linear function used to compute
  the single step length for scrolling.
  @return A pair [k, m] representing a function y(x) = k * x + m
*/
constexpr std::pair<float, float> computeSingleStepParameters()
{
  int t = DefaultSingleStepSize;
  int a = MinCameraZoom;
  int s = MinimumStepSizeInPixels;

  float m = (a * t - s) / (a - 1);
  float k = t - m;

  return {k, m};
}

constexpr std::pair<float, float> singleStepParams = computeSingleStepParameters();

int computeSingleStep(float zoom)
{
  const auto [k, m] = singleStepParams;
  double step = k * zoom + m;

  // Scale zoomed out step
  if (zoom > 1)
  {
    const double k = 0.2;
    double scaledStep = step * (3 * (zoom - 1) + (std::exp(k * zoom) - std::exp(k) + 1));

    return static_cast<int>(std::round(scaledStep));
  }
  else
  {
    return static_cast<int>(std::round(step));
  }
}
MapViewWidget::MapViewWidget(VulkanWindow *window, QWidget *parent)
    : QWidget(parent),
      MapView::Observer(window->getMapView()),
      mapView(window->getMapView()),
      vulkanWindow(window),
      hbar(new QtScrollBar(Qt::Horizontal)),
      vbar(new QtScrollBar(Qt::Vertical))
{
  {
    auto l = new BorderLayout;
    l->addWidget(hbar, BorderLayout::Position::South);
    l->addWidget(vbar, BorderLayout::Position::East);

    auto wrappedMapWindow = window->wrapInWidget();

    l->addWidget(wrappedMapWindow, BorderLayout::Position::Center);

    setLayout(l);
  }

  Map *map = mapView->map();
  uint16_t width = map->width();
  uint16_t height = map->width();

  int maxX = width * MapTileSize / MinimumStepSizeInPixels;
  int maxY = height * MapTileSize / MinimumStepSizeInPixels;

  hbar->setMinimum(0);
  hbar->setMaximum(maxX);
  hbar->setSingleStep(MapTileSize);

  vbar->setMinimum(0);
  vbar->setMaximum(maxY);
  vbar->setSingleStep(MapTileSize);

  connect(vulkanWindow, &VulkanWindow::keyPressedEvent, this, &MapViewWidget::onWindowKeyPress);

  connect(hbar, &QScrollBar::valueChanged, this, &MapViewWidget::setMapViewX);
  connect(vbar, &QScrollBar::valueChanged, this, &MapViewWidget::setMapViewY);
}

void MapViewWidget::setMapViewX(int value)
{
  auto *mapView = vulkanWindow->getMapView();
  if (mapView->cameraPosition().x != value)
  {
    mapView->setX(value);
  }
}

void MapViewWidget::setMapViewY(int value)
{
  auto *mapView = vulkanWindow->getMapView();
  if (mapView->cameraPosition().y != value)
  {
    mapView->setY(value);
  }
}

void MapViewWidget::onWindowKeyPress(QKeyEvent *event)
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

void MapViewWidget::viewportChanged(const Viewport &viewport)
{
  hbar->setValue(viewport.offset.x);
  vbar->setValue(viewport.offset.y);

  hbar->setPageStep(viewport.width);
  vbar->setPageStep(viewport.height);

  int singleStep = computeSingleStep(viewport.zoom);

  hbar->setSingleStep(singleStep);
  vbar->setSingleStep(singleStep);

  emit viewportChangedEvent(viewport);
}

void MapViewWidget::mapViewDrawRequested()
{
  vulkanWindow->requestUpdate();
}

/*
********************************
********************************
*QTScrollBar
********************************
********************************
*/

QtScrollBar::QtScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QScrollBar(orientation, parent) {}

void QtScrollBar::mousePressEvent(QMouseEvent *e)
{
  QStyleOptionSlider opt;
  initStyleOption(&opt);
  QStyle::SubControl subControl = style()->hitTestComplexControl(QStyle::CC_ScrollBar, &opt, e->pos(), this);
  bool scrollbarJump = subControl == QStyle::SC_ScrollBarAddPage || subControl == QStyle::SC_ScrollBarSubPage;

  if (!scrollbarJump)
  {
    QScrollBar::mousePressEvent(e);
    e->accept();
    VME_LOG_D("Accepted");
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

void QtScrollBar::initStyleOption(QStyleOptionSlider *option) const
{
  QScrollBar::initStyleOption(option);
}

void QtScrollBar::addValue(int value)
{
  setValue(this->value() + value);
}
