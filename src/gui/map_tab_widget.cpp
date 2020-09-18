#include "map_tab_widget.h"

#include <QWidget>
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include <QPushButton>
#include <QSvgWidget>
#include <QStyle>
#include <QStyleOptionTab>
#include <QHBoxLayout>
#include <QPainter>
#include <QLayout>
#include <QSvgRenderer>
#include <QFrame>
#include <QLabel>
#include <QRect>
#include <QEvent>

#include <QDesktopWidget>
#include <QApplication>

#include "../logger.h"
#include "../qt/logging.h"

constexpr int DragTriggerDistancePixels = 5;

constexpr int CloseIconSize = 8;

MapTabWidget::MapTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    MapTabBar *tabBar = new MapTabBar(this);
    setTabBar(tabBar);

    setTabsClosable(true);
    connect(tabBar, &MapTabBar::tabCloseRequested, this, &MapTabWidget::closeTab);
}

int MapTabWidget::addTabWithButton(QWidget *widget, const QString &text, QVariant data)
{
    int index = addTab(widget, text);
    tabBar()->setTabData(index, data);

    SvgWidget *svg = new SvgWidget("resources/svg/close.svg");

    tabBar()->setTabButton(index, QTabBar::ButtonPosition::RightSide, svg);
    if (index != 0)
    {
        reinterpret_cast<MapTabBar *>(tabBar())->setCloseButtonVisible(index, false);
    }

    return index;
}

void MapTabWidget::closeTab(int index)
{
    QVariant &&data = tabBar()->tabData(index);

    widget(index)->deleteLater();
    removeTab(index);

    emit mapTabClosed(index, std::move(data));
}

/*
********************************************************
********************************************************
* MapTabBar
********************************************************
********************************************************
*/
MapTabWidget::MapTabBar::MapTabBar(MapTabWidget *parent) : QTabBar(parent)
{
    setCursor(Qt::PointingHandCursor);
    new QHBoxLayout(this);
    setMouseTracking(true);

    // Does nothing?
    layout()->setSpacing(0);
    setContentsMargins(0, 0, 0, 0);
    updateGeometry();

    connect(parent, &MapTabWidget::mapTabClosed, [=](int removedTabIndex) { removedTabEvent(removedTabIndex); });

    connect(this, &QTabBar::currentChanged, [this](int index) {
        VME_LOG_D("Active tab changed from " << this->prevActiveIndex << " to " << index);
        if (this->prevActiveIndex != -1)
        {
            this->setCloseButtonVisible(prevActiveIndex, false);
        }
        this->prevActiveIndex = index;
        if (index != -1)
        {
            this->setCloseButtonVisible(index, true);
        }
    });
}

void MapTabWidget::MapTabBar::removedTabEvent(int removedIndex)
{
    QPoint localPos = mapFromGlobal(QCursor::pos());

    int hoveredTabIndex = tabAt(localPos);
    setHoveredIndex(hoveredTabIndex);
}

bool MapTabWidget::MapTabBar::intersectsCloseButton(QPoint point, int index) const
{
    if (index == -1)
        return false;

    auto button = tabButton(index, QTabBar::ButtonPosition::RightSide);

    QRect adjustedRect(button->rect());
    // ![Ugly] QTabBar adds 5px of padding to the right of the button. The bounding rect of the button is increased here to compensate for that.
    int unwantedPadding = 5;
    adjustedRect.setRight(adjustedRect.right() + unwantedPadding);

    bool intersectsButton = adjustedRect.contains(point - button->pos());

    return intersectsButton;
}

bool MapTabWidget::MapTabBar::intersectsCloseButton(QPoint point) const
{
    return intersectsCloseButton(point, tabAt(point));
}

bool MapTabWidget::MapTabBar::withinWidget(QPoint relativePoint) const
{
    return rect().contains(relativePoint);
}

void MapTabWidget::MapTabBar::mousePressEvent(QMouseEvent *event)
{
    // VME_LOG_D("MapTabWidget::MapTabBar::mousePressEvent");

    if (intersectsCloseButton(event->pos()))
    {
        closePendingIndex = tabAt(event->pos());
    }
    else
    {
        event->ignore();
        QTabBar::mousePressEvent(event);

        activePressPos = event->pos();
    }
}

void MapTabWidget::MapTabBar::setCloseButtonVisible(int index, bool visible)
{
    auto button = tabButton(index, QTabBar::ButtonPosition::RightSide);
    if (button)
    {
        if (visible)
        {
            // VME_LOG_D("Showing " << index << " (" << button << ")");
            button->show();
        }
        else
        {
            // Close button for current tab must always be visible
            if (index != currentIndex())
            {
                button->hide();
            }
        }
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
            closePendingIndex = -1;
            activePressPos.reset();

            auto *drag = new QDrag(getActiveWidget());

            auto data = new QMimeData;
            drag->setMimeData(data);
            drag->exec();
        }
    }
    else
    {
        QWidget *hoveredButton = tabButton(hoveredIndex, QTabBar::ButtonPosition::RightSide);
        QPoint localPos = mapFromGlobal(QCursor::pos());
        if (!withinWidget(localPos))
        {
            setHoveredIndex(-1);
        }
        else
        {
            int hoveredTabIndex = tabAt(localPos);
            setHoveredIndex(hoveredTabIndex);
        }
    }
}

void MapTabWidget::MapTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    // VME_LOG_D("MapTabWidget::MapTabBar::mouseReleaseEvent");
    if (closePendingIndex != -1 && intersectsCloseButton(event->pos(), closePendingIndex))
    {
        emit tabCloseRequested(hoveredIndex);
    }
    closePendingIndex = -1;

    event->ignore();
    QTabBar::mouseReleaseEvent(event);

    activePressPos.reset();
}

void MapTabWidget::MapTabBar::enterEvent(QEvent *event)
{

    // tabButton(currentIndex(), QTabBar::ButtonPosition::RightSide)->show();
}
void MapTabWidget::MapTabBar::leaveEvent(QEvent *event)
{
    if (hoveredIndex != -1 && hoveredIndex != currentIndex())
    {
        setCloseButtonVisible(hoveredIndex, false);
    }
}

MapTabWidget *MapTabWidget::MapTabBar::parentWidget() const
{
    return static_cast<MapTabWidget *>(parent());
}

QWidget *MapTabWidget::MapTabBar::getActiveWidget()
{
    return parentWidget()->widget(this->currentIndex());
}

void MapTabWidget::MapTabBar::setHoveredIndex(int index)
{
    if (hoveredIndex != index)
    {
        setCloseButtonVisible(hoveredIndex, false);
    }

    if (index != -1)
    {
        setCloseButtonVisible(index, true);
    }

    hoveredIndex = index;
}

bool MapTabWidget::MapTabBar::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::HoverLeave:
    {
        VME_LOG_D("HoverLeave. hoveredIndex: " << hoveredIndex << ", currentIndex: " << currentIndex());
        if (hoveredIndex != currentIndex())
        {
            setCloseButtonVisible(hoveredIndex, false);
        }

        setHoveredIndex(-1);
    }

    break;
    default:
        break;
    }

    return QTabBar::event(event);
}

/*
********************************************************
********************************************************
* SvgWidget
********************************************************
********************************************************
*/
SvgWidget::SvgWidget(const QString &file, QWidget *parent) : QFrame(parent)
{
    setFrameStyle(QFrame::NoFrame);
    {
        svg = new QSvgWidget(file);
        svg->setObjectName("my-svg");
        auto layout = new QHBoxLayout(svg);
        layout->setMargin(0);
        layout->setSpacing(0);
    }

    svg->setFixedSize(CloseIconSize, CloseIconSize);

    // setLayout(layout);

    auto layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->addWidget(svg, 0, 0, 1, 1);
    layout->setSizeConstraint(QLayout::SetFixedSize);
}

bool SvgWidget::event(QEvent *event)
{
    if (event->type() == QEvent::StyleChange)
    {
    }

    return QFrame::event(event);
}
