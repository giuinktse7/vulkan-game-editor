#include "../graphics/appearances.h"

#include <sstream>

#include <QPoint>
#include <QPointF>
#include <QRect>

std::ostream &operator<<(std::ostream &os, QPoint point)
{
  os << "{ x: " << point.x() << ", y: " << point.y() << " }";
  return os;
}

std::ostream &operator<<(std::ostream &os, QSize size)
{
  os << "{ width: " << size.width() << ", height: " << size.height() << " }";
  return os;
}

std::ostream &operator<<(std::ostream &os, QPointF point)
{
  os << "{ x: " << point.x() << ", y: " << point.y() << " }";
  return os;
}

std::ostream &operator<<(std::ostream &os, QRect rect)
{
  os << "{ top: " << rect.top() << ", right: " << rect.right() << ", bottom: " << rect.bottom() << ", left: " << rect.left() << " }";
  return os;
}

std::ostream &operator<<(std::ostream &os, QMargins margins)
{
  os << "{ top: " << margins.top() << ", right: " << margins.right() << ", bottom: " << margins.bottom() << ", left: " << margins.left() << " }";
  return os;
}
