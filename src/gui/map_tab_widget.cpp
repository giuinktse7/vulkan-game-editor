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

#include <QPaintEvent>
#include <QStylePainter>

#include <QDesktopWidget>
#include <QApplication>

#include "../logger.h"
#include "../qt/logging.h"

#include "../debug.h"
#include "../util.h"

constexpr int CloseIconSize = 8;

MapTabWidget::MapTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    MapTabBar *mapTabBar = new MapTabBar(this);
    setTabBar(mapTabBar);
    mapTabBar->setExpanding(false);

    setTabsClosable(true);
    connect(mapTabBar, &MapTabBar::tabCloseRequested, this, &MapTabWidget::closeTab);
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
    // auto vlayout = new QVBoxLayout;

    // setLayout(vlayout);

    scrollBar = new QtScrollBar(Qt::Orientation::Horizontal);
    // scrollBar->setStyle(style());
    // scrollBar->setStyleSheet(styleSheet());
    scrollBar->setObjectName("map-tabbar-scrollbar");
    scrollBar->setFixedWidth(500);
    scrollBar->setProperty("mapTabBar", true);
    // vlayout->addWidget(bar);

    // auto l = new QHBoxLayout;
    // setLayout(l);
    setMouseTracking(true);
    setAcceptDrops(true);
    setUsesScrollButtons(false);
    // setStyle(new TestProxyStyle);

    connect(parent, &MapTabWidget::mapTabClosed, [=](int removedTabIndex) { removedTabEvent(removedTabIndex); });

    connect(this, &QTabBar::currentChanged, [this](int index) {
        // VME_LOG_D("Active tab changed from " << this->prevActiveIndex << " to " << index);
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

    auto pos = event->pos();

    if (event->button() == Qt::LeftButton)
    {
        if (intersectsCloseButton(event->pos()))
        {
            closePendingIndex = tabAt(event->pos());
        }
        else
        {

            dragStartPosition = pos;
        }
    }

    event->ignore();
    QTabBar::mousePressEvent(event);
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

    if (tabAt(event->pos()) != -1)
    {
        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }

    if (pressed())
    {
        if ((event->pos() - dragStartPosition.value())
                .manhattanLength() < QApplication::startDragDistance())
            return;

        closePendingIndex = -1;
        dragStartPosition.reset();

        int currentTab = tabAt(event->pos());

        QObject *dragSource = this;
        auto *drag = new QDrag(dragSource);

        auto data = new MapTabMimeData;
        data->setInt(currentTab);

        drag->setMimeData(data);

        Qt::DropAction drop = drag->exec(Qt::MoveAction);
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
    if (closePendingIndex != -1 && intersectsCloseButton(event->pos(), closePendingIndex))
    {
        emit tabCloseRequested(hoveredIndex);
    }
    closePendingIndex = -1;

    event->ignore();
    QTabBar::mouseReleaseEvent(event);

    dragStartPosition.reset();
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

void MapTabWidget::MapTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(MapTabMimeData::integerMimeType()))
    {
        setDragHoveredIndex(tabAt(event->pos()));
        event->acceptProposedAction();
    }
}

void MapTabWidget::MapTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(MapTabMimeData::integerMimeType()))
    {
        event->acceptProposedAction();

        auto pos = event->pos();
        int tabIndex = tabAt(pos);

        setDragHoveredIndex(tabIndex);
    }
}

void MapTabWidget::MapTabBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    dragHoverIndex.reset();
    update();
}

void MapTabWidget::MapTabBar::dropEvent(QDropEvent *event)
{
    // Only accept drops from MapTabWidget::MapTabBar*
    if (!dynamic_cast<MapTabWidget::MapTabBar *>(event->source()))
        return;

    dragHoverIndex.reset();
    auto mimeData = static_cast<const MapTabMimeData *>(event->mimeData());

    int srcIndex = mimeData->getInt();
    int cursorTabIndex = tabAt(event->pos());

    // Moved internally
    if (this == event->source())
    {
        int destIndex = cursorTabIndex;
        if (destIndex == -1)
            destIndex = count() - 1;

        if (srcIndex != destIndex)
        {
            moveTab(srcIndex, destIndex);
        }
        else
        {
            // Required to update the drag hover rectangle
            update();
        }
    }
    else
    {
        if (cursorTabIndex == -1)
        {
            MapTabWidget *p = parentWidget();

            p->addTabWithButton(p->widget(srcIndex), p->tabText(srcIndex));
            // p->removeTab(droppedTabIndex);
        }
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

void MapTabWidget::MapTabBar::setDragHoveredIndex(int index)
{
    if (util::contains(dragHoverIndex, index))
        return;

    dragHoverIndex = index;
    update();
}

bool MapTabWidget::MapTabBar::event(QEvent *event)
{
    switch (event->type())
    {
    case QEvent::HoverLeave:
    {
        // VME_LOG_D("HoverLeave. hoveredIndex: " << hoveredIndex << ", currentIndex: " << currentIndex());
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

MapTabMimeData::MapTabMimeData() : QMimeData()
{
}

void MapTabMimeData::setInt(int value)
{
    QByteArray byteArray;
    QDataStream dataStream(&byteArray, QIODevice::WriteOnly);
    dataStream << value;

    setData(MapTabMimeData::integerMimeType(), byteArray);
}

int MapTabMimeData::getInt() const
{
    QByteArray byteArray = data(integerMimeType());
    QDataStream dataStream(&byteArray, QIODevice::ReadOnly);
    int value;
    dataStream >> value;

    return value;
}

bool MapTabMimeData::hasFormat(const QString &mimeType) const
{
    return QMimeData::hasFormat(mimeType) || mimeType == integerMimeType();
}
QStringList MapTabMimeData::formats() const
{
    QStringList formats = QMimeData::formats();
    formats.append(integerMimeType());
    return formats;
}

QVariant MapTabMimeData::retrieveData(const QString &mimeType, QVariant::Type type) const
{
    if (QMimeData::hasFormat(mimeType))
    {
        return QMimeData::retrieveData(mimeType, type);
    }

    if (mimeType == integerMimeType())
    {
        QDataStream buffer(data(mimeType));
        int x;
        buffer >> x;
        return x;
    }
    else
    {
        VME_LOG("Unknown mimeType in retrieveData: " << mimeType);
        ABORT_PROGRAM("Unknown mimeType in retrieveData.");
    }
}

void MapTabWidget::MapTabBar::paintEvent(QPaintEvent *event)
{
    QStyleOptionTabBarBase optTabBase;
    QStylePainter painter(this);

    QColor dragHoverColor("#CEDCEC");

    for (int i = 0; i < count(); ++i)
    {
        QStyleOptionTab tab;
        initStyleOption(&tab, i);
        if (!(tab.state & QStyle::State_Enabled))
        {
            tab.palette.setCurrentColorGroup(QPalette::Disabled);
        }
        // If this tab is partially obscured, make a note of it so that we can pass the information
        // along when we draw the tear.
        QRect rect = tabRect(i);
        // Don't bother drawing a tab if the entire tab is outside of the visible tab bar.
        if ((tab.rect.right() < 0 || tab.rect.left() > width()))
            continue;

        // tab.palette.setColor(backgroundRole(), QColor(255, 0, 0));
        // tab.palette.setColor(QPalette::Base, QColor(255, 0, 0));
        // tab.palette.setColor(QPalette::Button, QColor(255, 0, 0));
        // painter.drawControl(QStyle::CE_TabBarTab, tab);
        if (dragHoverIndex.has_value() && i == dragHoverIndex.value())
        {
            painter.fillRect(rect, dragHoverColor);
        }
        if (tab.state & QStyle::State_Selected)
        {
            painter.fillRect(rect, QColor("#FFFFFF"));
        }
        painter.drawControl(QStyle::CE_TabBarTabShape, tab);
        painter.drawControl(QStyle::CE_TabBarTabLabel, tab);
    }

    if (dragHoverIndex.has_value() && dragHoverIndex.value() == -1 && currentIndex() != count() - 1)
    {
        QRect last = tabRect(count() - 1);
        QRect highlightRect = QRect(last.right(), last.top(), rect().right() - last.right(), height());

        painter.fillRect(highlightRect, dragHoverColor);
    }

    painter.translate(0, height() - 8);
    QStyleOptionSlider opt;
    scrollBar->initStyleOption(&opt);
    opt.subControls = QStyle::SC_ScrollBarSlider | QStyle::SC_ScrollBarGroove;
    style()->drawComplexControl(QStyle::CC_ScrollBar, &opt, &painter, scrollBar);
}

void TestProxyStyle::drawControl(ControlElement element,
                                 const QStyleOption *option,
                                 QPainter *painter,
                                 const QWidget *widget) const
{
    switch (element)
    {
    case CE_TabBarTab:
    {
        QStyleOptionButton myButtonOption;

        const QStyleOptionButton *buttonOption = qstyleoption_cast<const QStyleOptionButton *>(option);
        QProxyStyle::drawControl(element, &myButtonOption, painter, widget);
    }
    break;
    default:
        QProxyStyle::drawControl(element, option, painter, widget);
    }
}