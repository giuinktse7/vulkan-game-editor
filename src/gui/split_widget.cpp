#include "split_widget.h"

#include <QPainter>
#include <QStyleOption>

#include "../logger.h"

Splitter::Splitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(parent)
{
    setOrientation(orientation);
    setChildrenCollapsible(false);
    setHandleWidth(4);
}

QSplitterHandle *Splitter::createHandle()
{
    auto *handle = new SplitterHandle(orientation(), this);
    switch (orientation())
    {
        case Qt::Orientation::Horizontal:
            handle->setCursor(Qt::SizeHorCursor);
            break;
        case Qt::Orientation::Vertical:
            handle->setCursor(Qt::SizeVerCursor);
            break;
    }

    return handle;
}

SplitterHandle::SplitterHandle(Qt::Orientation orientation, QSplitter *parent)
    : QSplitterHandle(orientation, parent) {}

void SplitterHandle::resizeEvent(QResizeEvent *event)
{
    setAttribute(Qt::WA_MouseNoMask, true);

    if (orientation() == Qt::Horizontal)
        setContentsMargins(3, 0, 3, 0);
    else
        setContentsMargins(0, 3, 0, 3);

    setMask(QRegion(contentsRect()));

    QWidget::resizeEvent(event);
}

void SplitterHandle::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    QStyleOption opt(0);

    opt.rect = contentsRect();
    opt.palette = palette();

    if (orientation() == Qt::Horizontal)
        opt.state = QStyle::State_Horizontal;
    else
        opt.state = QStyle::State_None;
    if (this->underMouse())
        opt.state |= QStyle::State_MouseOver;
    // if (d->pressed)
    // opt.state |= QStyle::State_Sunken;
    if (isEnabled())
        opt.state |= QStyle::State_Enabled;
    parentWidget()->style()->drawControl(QStyle::CE_Splitter, &opt, &p, this);
}