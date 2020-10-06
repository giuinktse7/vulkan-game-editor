#pragma once

#include <optional>

#include <QWidget>
#include <QPixmap>
#include <QString>
#include <QMouseEvent>
#include <QApplication>

#include "../input.h"

#include "../qt/enum_conversion.h"

class MainApplication;

class QWheelEvent;
class MapView;
class VulkanWindow;

namespace
{
  constexpr int QtMinimumWheelDelta = 8;
  constexpr int DefaultMinimumAngleDelta = 15;
} // namespace

namespace QtUtil
{
  class QtUiUtils : public UIUtils
  {
  public:
    QtUiUtils(VulkanWindow *window) : window(window) {}
    ScreenPosition mouseScreenPosInView() override;

    VME::ModifierKeys modifiers() const override;

  private:
    VulkanWindow *window;
  };

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
    VME::MouseButtons buttons = enum_conversion::vmeButtons(event->buttons());
    VME::ModifierKeys modifiers = enum_conversion::vmeModifierKeys(event->modifiers());

    ScreenPosition pos(event->pos().x(), event->pos().y());
    return VME::MouseEvent(pos, buttons, modifiers);
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
