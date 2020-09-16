#include "map_tab_widget.h"

#include <QWidget>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include <QPushButton>
#include <QSvgWidget>
#include <QStyle>
#include <QStyleOptionTab>

#include "../logger.h"
#include "../qt/logging.h"

constexpr int DragTriggerDistancePixels = 5;

constexpr int CloseIconSize = 8;

MapTabWidget::MapTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
  MapTabBar *tabBar = new MapTabBar;
  setTabBar(tabBar);

  setTabsClosable(true);
  connect(tabBar, &MapTabBar::tabCloseRequested, this, &MapTabWidget::closeTab);
}

int MapTabWidget::addTabWithButton(QWidget *widget, const QString &text)
{
  int index = addTab(widget, text);

  auto svg = new QSvgWidget;
  const QString f = "resources/svg/close.svg";
  svg->load(f);
  svg->setMaximumSize(CloseIconSize, CloseIconSize);
  svg->setMinimumSize(CloseIconSize, CloseIconSize);

  tabBar()->setTabButton(index, QTabBar::ButtonPosition::RightSide, svg);

  return index;
}

void MapTabWidget::closeTab(int index)
{
  widget(index)->deleteLater();
  removeTab(index);
}

bool MapTabWidget::MapTabBar::intersectsCloseButton(QPoint pos)
{
  int index = currentIndex();
  auto button = tabButton(index, QTabBar::ButtonPosition::RightSide);

  VME_LOG_D("pos - button->pos(): " << (pos - button->pos()));

  return button->rect().contains(pos - button->pos());
}

void MapTabWidget::MapTabBar::mousePressEvent(QMouseEvent *event)
{
  VME_LOG_D("MapTabWidget::MapTabBar::mousePressEvent");
  event->ignore();
  QTabBar::mousePressEvent(event);

  if (intersectsCloseButton(event->pos()))
  {
    closePending = true;
  }
  else
  {
    activePressPos = event->pos();
  }
}

void MapTabWidget::MapTabBar::mouseMoveEvent(QMouseEvent *event)
{
  event->ignore();
  QTabBar::mouseMoveEvent(event);

  if (pressed())
  {
    QPoint diff = event->pos() - activePressPos.value();
    if (diff.manhattanLength() > DragTriggerDistancePixels)
    {
      closePending = false;
      activePressPos.reset();

      auto *drag = new QDrag(getActiveWidget());

      auto data = new QMimeData;
      drag->setMimeData(data);
      drag->exec();
    }
  }
}

void MapTabWidget::MapTabBar::mouseReleaseEvent(QMouseEvent *event)
{
  VME_LOG_D("MapTabWidget::MapTabBar::mouseReleaseEvent");

  if (closePending && intersectsCloseButton(event->pos()))
  {
    parentWidget()->removeTab(currentIndex());
  }
  closePending = false;

  event->ignore();
  QTabBar::mouseReleaseEvent(event);

  activePressPos.reset();
}

MapTabWidget *MapTabWidget::MapTabBar::parentWidget() const
{
  return static_cast<MapTabWidget *>(parent());
}

QWidget *MapTabWidget::MapTabBar::getActiveWidget()
{
  return parentWidget()->widget(this->currentIndex());
}
