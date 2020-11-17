#pragma once

#include <optional>

#include <QApplication>
#include <QMouseEvent>
#include <QPixmap>
#include <QString>
#include <QWidget>

#include "../editor_action.h"

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

    void waitForDraw(std::function<void()> f) override;

  private:
    VulkanWindow *window;
  };

  namespace PropertyName
  {
    constexpr const char *MapView = "vme-mapview";
    constexpr const char *VulkanWindow = "vme-vulkan-window";
  } // namespace PropertyName

  template <typename T>
  inline QVariant wrapPointerInQVariant(T *pointer)
  {
    return QVariant::fromValue(static_cast<void *>(pointer));
  }

  inline void setMapView(QWidget &widget, MapView *mapView)
  {
    widget.setProperty(PropertyName::MapView, wrapPointerInQVariant(mapView));
  }

  inline void setVulkanWindow(QWidget &widget, VulkanWindow *vulkanWindow)
  {
    widget.setProperty(PropertyName::VulkanWindow, wrapPointerInQVariant(vulkanWindow));
  }

  MapView *associatedMapView(QWidget *widget);
  VulkanWindow *associatedVulkanWindow(QWidget *widget);

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

  QPixmap itemPixmap(uint32_t serverId);
  MainApplication *qtApp();

  class EventFilter : public QObject
  {
  public:
    EventFilter(QObject *parent) : QObject(parent) {}

  protected:
    virtual bool eventFilter(QObject *obj, QEvent *event) = 0;
  };
} // namespace QtUtil

template <typename T>
inline QString toQString(T value)
{
  std::ostringstream s;
  s << value;
  return QString::fromStdString(s.str());
}
