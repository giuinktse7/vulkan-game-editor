#include "border_layout.h"

#include <QWidget>

#include "../logger.h"
#include "../qt/logging.h"

BorderLayout::BorderLayout(QWidget *parent, const QMargins &margins, int spacing)
    : QLayout(parent)
{
  setContentsMargins(margins);
  setSpacing(spacing);
}

BorderLayout::BorderLayout(int spacing)
{
  setSpacing(spacing);
}

BorderLayout::~BorderLayout()
{
  for (auto &wrapper : items)
    delete wrapper.item;
}

void BorderLayout::addItem(QLayoutItem *item)
{
  add(item, Position::West);
}

void BorderLayout::addWidget(QWidget *widget, Position position)
{
  add(new QWidgetItem(widget), position);
}

Qt::Orientations BorderLayout::expandingDirections() const
{
  return Qt::Horizontal | Qt::Vertical;
}

bool BorderLayout::hasHeightForWidth() const
{
  return false;
}

int BorderLayout::count() const
{
  return items.size();
}

QLayoutItem *BorderLayout::itemAt(int index) const
{
  if (0 <= index && index < items.size())
    return items.at(index).item;

  return nullptr;
}

QSize BorderLayout::minimumSize() const
{
  return calculateSize(SizeType::MinimumSize);
}

void BorderLayout::setGeometry(const QRect &rect)
{
  ItemWrapper *center = nullptr;

  int eastWidth = 0;
  int westWidth = 0;
  int northHeight = 0;
  int southHeight = 0;
  int centerHeight = 0;

  if (this->menuBar())
  {
    int menuHeight = this->menuBar()->geometry().height();
    northHeight += menuHeight;
  }

  QLayout::setGeometry(rect);

  for (auto &wrapper : items)
  {
    const auto [item, position] = wrapper;
    // VME_LOG_D("margins: " << item->widget()->contentsMargins());
    switch (position)
    {
    case Position::North:
    {
      item->setGeometry(QRect(rect.x(), northHeight, rect.width(),
                              item->sizeHint().height()));

      northHeight += item->geometry().height() + spacing();
      break;
    }
    case Position::South:
      item->setGeometry(QRect(item->geometry().x(),
                              item->geometry().y(), rect.width(),
                              item->sizeHint().height()));

      southHeight += item->geometry().height() + spacing();

      item->setGeometry(QRect(rect.x(),
                              rect.y() + rect.height() - southHeight + spacing(),
                              item->geometry().width(),
                              item->geometry().height()));
      break;
    case Position::Center:
      center = &wrapper;
      break;
    default:
      break;
    }
  }

  centerHeight = rect.height() - northHeight - southHeight;

  for (const auto [item, position] : items)
  {
    if (position == Position::West)
    {
      QRect geometry(
          rect.x() + westWidth,
          northHeight,
          item->sizeHint().width(),
          centerHeight);
      item->setGeometry(geometry);

      westWidth += item->geometry().width() + spacing();
    }
    else if (position == Position::East)
    {
      {
        QRect geometry(
            item->geometry().x(),
            item->geometry().y(),
            item->sizeHint().width(),
            centerHeight);
        item->setGeometry(geometry);
      }

      eastWidth += item->geometry().width() + spacing();

      {
        QRect geometry(
            rect.x() + rect.width() - eastWidth + spacing(),
            northHeight,
            item->geometry().width(),
            item->geometry().height());
        item->setGeometry(geometry);
      }
    }
  }

  if (center)
    center->item->setGeometry(QRect(westWidth, northHeight,
                                    rect.width() - eastWidth - westWidth,
                                    centerHeight));
}

QSize BorderLayout::sizeHint() const
{
  return calculateSize(SizeType::SizeHint);
}

QLayoutItem *BorderLayout::takeAt(int index)
{
  if (0 < index && index < items.size())
    return items.at(index).item;

  return nullptr;
}

void BorderLayout::add(QLayoutItem *item, Position position)
{
  items.emplace_back(item, position);
}

QSize BorderLayout::calculateSize(SizeType sizeType) const
{
  QSize totalSize;

  for (const auto wrapper : items)
  {
    const auto [item, position] = wrapper;
    QSize itemSize;

    if (sizeType == SizeType::MinimumSize)
      itemSize = wrapper.item->minimumSize();
    else // (sizeType == SizeType::SizeHint)
      itemSize = wrapper.item->sizeHint();

    if (position == Position::North || position == Position::South || position == Position::Center)
      totalSize.rheight() += itemSize.height();

    if (position == Position::West || position == Position::East || position == Position::Center)
      totalSize.rwidth() += itemSize.width();
  }

  return totalSize;
}
