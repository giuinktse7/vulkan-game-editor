#pragma once

#include <QPixmap>
#include <QString>
#include <QMouseEvent>

#include "gui.h"

class MainApplication;

class QWheelEvent;

namespace std
{
  template <typename>
  class optional;
}

namespace
{
  constexpr int QtMinimumWheelDelta = 8;
  constexpr int DefaultMinimumAngleDelta = 15;
} // namespace

namespace QtUtil
{
  struct ScrollState
  {
    ScrollState() {}
    ScrollState(int minimumAngleDelta) : minRotationDelta(minimumAngleDelta * QtMinimumWheelDelta) {}

    int amountBuffer = 0;

    std::optional<int> scroll(QWheelEvent *event);

  private:
    /*
      The minimum rotation amount for a scroll to be registered, in eighths of a degree.
      For example, 120 MinRotationAmount = (120 / 8) = 15 degrees of rotation.
    */
    int minRotationDelta = DefaultMinimumAngleDelta * QtMinimumWheelDelta;
  };

  inline VME::MouseButtons vmeMouseEvent(QMouseEvent *event)
  {
    VME::MouseButtons buttons = VME::MouseButtons::NoButton;

    if (event->buttons() & Qt::LeftButton)
    {
      buttons |= VME::MouseButtons::LeftButton;
    }
    if (event->buttons() & Qt::RightButton)
    {
      buttons |= VME::MouseButtons::RightButton;
    }

    return buttons;
  }

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