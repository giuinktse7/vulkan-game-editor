#pragma once

#include <sstream>

class QPoint;
class QPointF;
class QRect;

std::ostream &operator<<(std::ostream &os, QPoint point);
std::ostream &operator<<(std::ostream &os, QPointF point);
std::ostream &operator<<(std::ostream &os, QRect rect);
