#pragma once

#include <QPixmap>
#include <QString>

class MainApplication;

namespace QtUtil
{
  QPixmap itemPixmap(uint16_t serverId);
  MainApplication *qtApp();

} // namespace QtUtil

template <typename T>
inline QString toQString(T value)
{
  std::ostringstream s;
  s << value;
  return QString::fromStdString(s.str());
}