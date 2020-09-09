#include "map_view_widget.h"

#include <QScrollBar>
#include <QSize>

QtMapViewWidget::QtMapViewWidget(VulkanWindow *window, QWidget *parent)
    : QAbstractScrollArea(parent)
{
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  setViewport(window->wrapInWidget());

  horizontalScrollBar()->setMinimum(0);
  horizontalScrollBar()->setMaximum(2048);
  horizontalScrollBar()->setValue(2048 / 2);

  verticalScrollBar()->setMinimum(0);
  verticalScrollBar()->setMaximum(2048);
  verticalScrollBar()->setValue(2048 / 2);

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
  // TODO
}

void QtMapViewWidget::resizeEvent(QResizeEvent *event)
{
  // TODO
  horizontalScrollBar()->update();
  verticalScrollBar()->update();
  viewport()->update();
}
