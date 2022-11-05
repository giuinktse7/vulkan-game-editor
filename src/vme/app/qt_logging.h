#pragma once

#include <QString>

#include <sstream>

class QPoint;
class QPointF;
class QRect;
class QSize;
class QMargins;

std::ostream &operator<<(std::ostream &os, QPoint point);
std::ostream &operator<<(std::ostream &os, QSize point);
std::ostream &operator<<(std::ostream &os, QPointF point);
std::ostream &operator<<(std::ostream &os, QRect rect);
std::ostream &operator<<(std::ostream &os, QMargins margins);
std::ostream &operator<<(std::ostream &os, QString s);
