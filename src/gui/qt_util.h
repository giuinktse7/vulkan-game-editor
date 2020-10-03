#pragma once

#include <QWidget>
#include <QPixmap>
#include <QString>
#include <QMouseEvent>

#include "gui.h"

class MainApplication;

class QWheelEvent;
class MapView;

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
  namespace PropertyName
  {
    constexpr const char *MapView = "vme-mapview";
  }

  template <typename T>
  inline QVariant wrapPointerInQVariant(T *pointer)
  {
    return QVariant::fromValue(static_cast<void *>(pointer));
  }

  inline void associateWithMapView(QWidget &widget, MapView *mapView)
  {
    widget.setProperty(PropertyName::MapView, wrapPointerInQVariant(mapView));
  }

  MapView *associatedMapView(QWidget *widget);

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

  inline VME::MouseEvent vmeMouseEvent(QMouseEvent *event)
  {
    auto qtButtons = event->buttons();
    auto qtModifiers = event->modifiers();

    VME::MouseButtons buttons = VME::MouseButtons::NoButton;
    VME::ModifierKeys modifiers = VME::ModifierKeys::None;

    if (qtButtons & Qt::LeftButton)
      buttons |= VME::MouseButtons::LeftButton;

    if (qtButtons & Qt::RightButton)
      buttons |= VME::MouseButtons::RightButton;

    if (qtModifiers & Qt::CTRL)
      modifiers |= VME::ModifierKeys::Ctrl;

    if (qtModifiers & Qt::SHIFT)
      modifiers |= VME::ModifierKeys::Shift;

    if (qtModifiers & Qt::ALT)
      modifiers |= VME::ModifierKeys::Alt;

    return VME::MouseEvent(buttons, modifiers);
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