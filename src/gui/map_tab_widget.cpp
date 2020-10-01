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
#include <QAbstractAnimation>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QLayout>
#include <QSvgRenderer>
#include <QFrame>
#include <QLabel>
#include <QRect>
#include <QEvent>
#include <QSize>
#include <QTimer>

#include <QPaintEvent>
#include <QStylePainter>

#include <QDesktopWidget>
#include <QApplication>

#include "../logger.h"
#include "../qt/logging.h"

#include "../debug.h"
#include "../util.h"

namespace
{
    constexpr int CloseIconSize = 8;
    constexpr QSize MinSizeHint = ([]() { return QSize(400, 300); }());

    constexpr int DragScrollThresholdPx = 30;
    constexpr int DragScrollRefreshRateMs = 1000 / 60;
} // namespace

MapTabWidget::MapTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    MapTabBar *mapTabBar = new MapTabBar(this);
    setTabBar(mapTabBar);
    mapTabBar->setExpanding(false);

    setTabsClosable(true);
    connect(mapTabBar, &MapTabBar::tabCloseRequested, this, &MapTabWidget::closeTab);
}

MapView *MapTabWidget::getMapView(size_t index) const
{
    DEBUG_ASSERT(index < count(), "index must be less than size.");
    return static_cast<MapViewWidget *>(widget(index))->mapView;
}

MapView *MapTabWidget::currentMapView() const
{
    if (count() == 0)
        return nullptr;

    return getMapView(currentIndex());
}

int MapTabWidget::addTabWithButton(QWidget *widget, const QString &text, QVariant data)
{
    int index = insertTab(currentIndex() + 1, widget, text);
    tabBar()->setTabData(index, data);
    setCurrentIndex(index);

    reinterpret_cast<MapTabBar *>(tabBar())->initCloseButton(index);

    emit mapTabAdded(index);
    return index;
}

void MapTabWidget::closeTab(int index)
{
    QVariant &&data = tabBar()->tabData(index);

    widget(index)->deleteLater();
    removeTab(index);

    emit mapTabClosed(index, std::move(data));
}

void MapTabWidget::removeCurrentTab()
{
    closeTab(currentIndex());
}

QSize MapTabWidget::minimumSizeHint() const
{
    return MinSizeHint;
}

/*
********************************************************
********************************************************
* MapTabBar
********************************************************
********************************************************
*/
MapTabWidget::MapTabBar::MapTabBar(MapTabWidget *parent)
    : QTabBar(parent),
      scrollBar(new QtScrollBar(Qt::Orientation::Horizontal)),
      scrollBarAnimation(scrollBar),
      dragPanTimer(this)
{
    scrollBar->setProperty("mapTabBar", true);
    scrollBar->setSingleStep(1);
    scrollBar->setMinimum(0);

    {
        OpacityAnimation::AnimationData forward;
        forward.duration = 200;
        forward.startValue = 0.0;
        forward.endValue = 0.7;
        scrollBarAnimation.forward = forward;
    }

    {
        OpacityAnimation::AnimationData backward;
        backward.duration = 500;
        backward.startValue = 0.7;
        backward.endValue = 0.0;
        scrollBarAnimation.backward = backward;
    }

    dragPanTimer.setInterval(DragScrollRefreshRateMs);
    connect(&dragPanTimer, &QTimer::timeout, this, &MapTabWidget::MapTabBar::dragPanEvent);

    connect(&scrollBarAnimation, &OpacityAnimation::preShow, [=] {
    });
    connect(&scrollBarAnimation, &OpacityAnimation::postShow, [=] {
        QPoint localPos = mapFromGlobal(QCursor::pos());
        if (!(withinWidget(localPos) || scrollVisibilityState.hasTimer || drag))
        {
            VME_LOG_D("postShow hideWidget");
            this->scrollBarAnimation.hideWidget();
        }
    });

    // Render close button image
    QSvgWidget *closeButtonSvg = new QSvgWidget("resources/svg/close.svg");
    closeButtonSvg->setFixedSize(CloseIconSize, CloseIconSize);
    closeButtonImage = new QImage(closeButtonSvg->size(), QImage::Format_ARGB32);
    closeButtonImage->fill(Qt::transparent);
    QPainter p(closeButtonImage);
    closeButtonSvg->render(&p, QPoint(), QRegion(), QWidget::DrawChildren);

    connect(scrollBar, &QScrollBar::valueChanged, [=](int value) {
        this->scrollOffset = value;
        this->update();
    });

    auto l = new QHBoxLayout;
    setLayout(l);
    l->addWidget(scrollBar);
    l->insertWidget(0, scrollBar, 1, Qt::AlignLeft | Qt::AlignBottom);
    l->setMargin(0);
    l->setSpacing(0);

    scrollBar->hide();

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

    connect(parent, &MapTabWidget::mapTabAdded, this, &MapTabWidget::MapTabBar::updateScrollBarVisibility);
}

void MapTabWidget::MapTabBar::dragPanEvent()
{
    if (!tabOverflow())
        return;

    auto pos = mapFromGlobal(QCursor::pos());

    if (pos.x() < 0 || pos.x() > width())
        return;

    // Right side
    if (pos.x() > width() - DragScrollThresholdPx)
    {
        // int threshold = width() - pos.x();
        scrollBar->addValue(1);
    }
    // Left side
    else if (pos.x() < DragScrollThresholdPx)
    {
        // int threshold = pos.x();
        scrollBar->addValue(-1);
    }
}

void MapTabWidget::MapTabBar::updateScrollBarVisibility()
{
    if (!hasBeenShown || count() == 0)
        return;

    int lastRectRight = tabRect(count() - 1).right();

    if (lastRectRight > rect().right())
    {
        scrollBar->setFixedWidth(width());
        scrollBar->setMaximum(lastRectRight - width());

        // dragEdgePanAnimation.setEndValue(scrollBar->maximum());

        scrollBar->setPageStep(width());

        scrollBarAnimation.showWidget();
    }
    else
    {
        scrollBar->setValue(0);
        scrollBarAnimation.hideWidget();
    }
}

void MapTabWidget::MapTabBar::createScrollVisibilityTimer(time_t millis)
{
    scrollVisibilityState.hasTimer = true;
    QTimer::singleShot(millis, [=] {
        scrollVisibilityState.hasTimer = false;
        time_t millis = TimePoint::now().elapsedMillis(scrollVisibilityState.newTimerStart);
        if (millis >= 400)
        {
            // VME_LOG_D("resize hideWidget");
            scrollBarAnimation.hideWidget();
        }
        else
        {
            createScrollVisibilityTimer(400 - millis);
        }
    });
}

void MapTabWidget::MapTabBar::resizeEvent(QResizeEvent *event)
{
    event->ignore();
    QTabBar::resizeEvent(event);

    scrollVisibilityState.newTimerStart = TimePoint::now();
    if (!scrollVisibilityState.hasTimer)
    {
        createScrollVisibilityTimer(400);
    }

    VME_LOG_D("event->size(): " << event->size());
    updateScrollBarVisibility();
}

void MapTabWidget::MapTabBar::wheelEvent(QWheelEvent *event)
{
    if (scrollBar->isHidden())
        return;

    static const int DefaultScrollPxPerDegree = 3;
    auto scrollDegrees = tabBarScrollState.scroll(event);
    if (scrollDegrees.has_value())
    {
        int absDegrees = std::abs(scrollDegrees.value());
        int hiddenLength = tabRect(count() - 1).right() - this->width();

        int scrollValue = absDegrees * DefaultScrollPxPerDegree;
        scrollValue *= -util::sgn(scrollDegrees.value()) * 2 / std::log(hiddenLength);

        scrollBar->setValue(scrollBar->value() + scrollValue);
    }
}

void MapTabWidget::MapTabBar::removedTabEvent(int removedIndex)
{
    updateScrollBarVisibility();
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
    adjustedRect.moveLeft(-scrollOffset);

    bool intersectsButton = adjustedRect.contains(point - button->pos());

    return intersectsButton;
}

int MapTabWidget::MapTabBar::tabAt(const QPoint &pos) const
{
    return QTabBar::tabAt(QPoint(pos.x() + scrollOffset, pos.y()));
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
            int tabIndex = tabAt(pos);
            if (tabIndex == -1)
                return;

            setCurrentIndex(tabIndex);

            if (tabDragPixmaps.size() < tabIndex)
                tabDragPixmaps.resize(tabIndex + 1);

            QRect tabRectangle = tabRect(tabIndex);
            tabRectangle.moveLeft(tabRectangle.x() - scrollOffset);

            // tabDragPixmaps.emplace(tabDragPixmaps.begin() + tabIndex, grab(tabRectangle));

            dragStartPosition = pos;
        }
    }
}

QSize MapTabWidget::MapTabBar::minimumSizeHint() const
{
    return MinSizeHint;
}

void MapTabWidget::MapTabBar::setCloseButtonVisible(int index, bool visible)
{
    auto button = tabButton(index, QTabBar::ButtonPosition::RightSide);
    if (button)
    {
        if (visible)
        {
            // VME_LOG_D("Showing " << index << " (" << button << ")");
            // button->show();
        }
        else
        {
            button->hide();
        }
    }
}

void MapTabWidget::MapTabBar::mouseMoveEvent(QMouseEvent *event)
{

    // if (scrollBar->isHidden() || scrollAnimation)
    // {
    // }

    if (tabAt(event->pos()) != -1)
    {
        setCursor(Qt::PointingHandCursor);
    }
    else
    {
        setCursor(Qt::ArrowCursor);
    }

    if (dragPending())
    {
        closePendingIndex = -1;

        if ((event->pos() - dragStartPosition.value())
                .manhattanLength() < QApplication::startDragDistance())
            return;

        QPoint dragStartPos = dragStartPosition.value();

        int draggedTabIndex = tabAt(dragStartPos);
        auto tabRectangle = tabRect(draggedTabIndex);

        dragStartPosition.reset();

        drag = new QDrag(this);

        drag->setPixmap(QPixmap());

        auto data = new MapTabMimeData;
        data->setInt(draggedTabIndex);

        drag->setMimeData(data);

        drag->setHotSpot(QPoint(
            dragStartPos.x() - tabRectangle.x() + scrollOffset,
            dragStartPos.y() - tabRectangle.y()));

        drag->exec(Qt::MoveAction, Qt::MoveAction);
        drag = nullptr;
    }
    else
    {
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

        update();
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
    updateScrollBarVisibility();
}

void MapTabWidget::MapTabBar::leaveEvent(QEvent *event)
{
    if (!drag)
    {
        scrollBarAnimation.hideWidget();
    }

    if (hoveredIndex != -1 && hoveredIndex != currentIndex())
    {
        setCloseButtonVisible(hoveredIndex, false);
    }
}

void MapTabWidget::MapTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(MapTabMimeData::integerMimeType()))
    {
        event->acceptProposedAction();
        setDragHoveredIndex(tabAt(event->pos()));
    }
}

void MapTabWidget::MapTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasFormat(MapTabMimeData::integerMimeType()))
    {
        event->acceptProposedAction();

        if (!drag)
            return;

        auto pos = event->pos();

        if (tabOverflow())
        {
            bool panLeft = width() - pos.x() < DragScrollThresholdPx;
            bool panRight = pos.x() < DragScrollThresholdPx;
            if (panLeft || panRight)
            {
                if (!dragPanTimer.isActive())
                    dragPanTimer.start();
            }
            else
            {
                if (dragPanTimer.isActive())
                    dragPanTimer.stop();
            }
        }

        int tabIndex = tabAt(pos);
        setDragHoveredIndex(tabIndex);
    }
}

void MapTabWidget::MapTabBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    VME_LOG_D("Stopping timer.");
    dragPanTimer.stop();

    dragHoverIndex.reset();
    update();
}

bool MapTabWidget::MapTabBar::insertMoveTab(int srcIndex, int destIndex)
{
    if (destIndex == -1)
        destIndex = count() - 1;

    if (srcIndex == destIndex)
        return false;

    QWidget *srcWidget = parentWidget()->widget(srcIndex);
    QString srcText = tabText(srcIndex);
    // QWidget *closeButton = tabButton(srcIndex, QTabBar::ButtonPosition::RightSide);

    {
        // To reduce flicker. https://doc.qt.io/qt-5/qtabwidget.html#insertTab
        parentWidget()->setUpdatesEnabled(false);

        int i = parentWidget()->insertTab(destIndex, srcWidget, srcText);
        initCloseButton(i);
        parentWidget()->setCurrentIndex(i);
        parentWidget()->setUpdatesEnabled(true);
    }

    return true;
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

        bool tabMoved = insertMoveTab(srcIndex, destIndex);
        if (!tabMoved)
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

    drag = nullptr;
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
        // VME_LOG_D("Hovered: " << index);
        hoveredIndex = index;
        update();
    }
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

void MapTabWidget::MapTabBar::showEvent(QShowEvent *event)
{
    hasBeenShown = true;
    event->ignore();
    QTabBar::showEvent(event);
}

void MapTabWidget::MapTabBar::initCloseButton(int index)
{
    SvgWidget *svg = new SvgWidget("resources/svg/close.svg");

    setTabButton(index, QTabBar::ButtonPosition::RightSide, svg);

    // Close button rendering is handled in MapTabBar
    setCloseButtonVisible(index, false);
}

bool MapTabWidget::MapTabBar::tabOverflow() const
{
    return !(count() == 0 || tabRect(count() - 1).right() <= rect().right());
}

void MapTabWidget::MapTabBar::paintEvent(QPaintEvent *event)
{
    QStylePainter painter(this);

    QColor dragHoverColor("#CEDCEC");

    for (int i = 0; i < count(); ++i)
    {
        QStyleOptionTab tab;
        initStyleOption(&tab, i);

        if (this->scrollOffset > 0)
        {
            tab.rect.moveLeft(tab.rect.x() - scrollOffset);
        }

        // Don't bother drawing a tab if the entire tab is outside of the visible tab bar.
        if ((tab.rect.right() < 0 || tab.rect.left() > width()))
            continue;

        if (i == currentIndex())
        {
            // painter.fillRect(tab.rect, "blue");
            // painter.fillRect(tab.rect, QColor("white"));
            // painter.fillRect(tab.rect, QColor("red"));
            painter.fillRect(tab.rect, QColor(255, 255, 255, 255));
        }
        else if (dragHoverIndex.has_value() && i == dragHoverIndex.value())
        {
            painter.fillRect(tab.rect, dragHoverColor);
        }
        else
        {
            // painter.fillRect(tab.rect, QColor("red"));
            painter.fillRect(tab.rect, QColor("#FFECECEC"));
        }

        painter.drawControl(QStyle::CE_TabBarTabShape, tab);
        painter.drawControl(QStyle::CE_TabBarTabLabel, tab);
        if (i == hoveredIndex || i == currentIndex())
        {
            int rightMargin = 5;

            painter.drawImage(tab.rect.right() - closeButtonImage->width() - rightMargin, tab.rect.height() / 2 - closeButtonImage->height() / 2 + 1, *closeButtonImage);
        }
    }

    if (dragHoverIndex.has_value() && dragHoverIndex.value() == -1 && currentIndex() != count() - 1)
    {
        QRect last = tabRect(count() - 1);
        QRect highlightRect = QRect(last.right(), last.top(), rect().right() - last.right(), height());

        painter.fillRect(highlightRect, dragHoverColor);
    }
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

OpacityAnimation::OpacityAnimation() : widget(nullptr) {}

OpacityAnimation::OpacityAnimation(QWidget *widget)
    : widget(widget)
{
    forward.startValue = 0.0;
    forward.endValue = 1.0;

    backward.startValue = 1.0;
    backward.endValue = 0.0;

    widget->setGraphicsEffect(new QGraphicsOpacityEffect(this));
    animation = new QPropertyAnimation(widget->graphicsEffect(), "opacity");

    widget->connect(animation, &QPropertyAnimation::finished, [=] {
        AnimationState state = this->animationState;
        this->animationState = AnimationState::None;

        switch (state)
        {
        case AnimationState::Hiding:
            widget->hide();
            break;
        case AnimationState::Showing:
            emit postShow();
            break;
        default:
            break;
        }
    });
}

void OpacityAnimation::showWidget()
{
    switch (animationState)
    {
    case AnimationState::None:
    {
        if (!widget->isHidden())
            return;

        emit preShow();
        widget->show();
        animationState = AnimationState::Showing;
        animation->setDuration(forward.duration);
        animation->setStartValue(forward.startValue);
        animation->setEndValue(forward.endValue);
        animation->start();
        break;
    }
    case AnimationState::Showing:
    {
        break;
    }
    case AnimationState::Hiding:
    {
        animationState = AnimationState::Showing;
        int value = animation->currentValue().toInt();
        animation->pause();

        animation->setDuration(forward.duration);
        animation->setStartValue(forward.startValue);
        animation->setEndValue(forward.endValue);

        animation->resume();
        break;
    }
    }
}

void OpacityAnimation::hideWidget()
{
    switch (animationState)
    {
    case AnimationState::None:
    {
        if (widget->isHidden())
            return;

        animationState = AnimationState::Hiding;
        animation->setDuration(backward.duration);
        animation->setStartValue(backward.startValue);
        animation->setEndValue(backward.endValue);
        animation->start();
        break;
    }
    case AnimationState::Showing:
    {
        animationState = AnimationState::Hiding;
        int value = animation->currentValue().toInt();
        animation->pause();

        animation->setDuration(backward.duration);
        animation->setStartValue(backward.startValue);
        animation->setEndValue(backward.endValue);

        animation->resume();

        break;
    }
    case AnimationState::Hiding:
    {
        break;
    }
    }
}