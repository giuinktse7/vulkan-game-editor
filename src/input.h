#pragma once

#include <variant>

#include "util.h"
#include "position.h"

namespace VME
{

  enum MouseButtons
  {
    NoButton = 0,
    LeftButton = 1 << 0,
    RightButton = 1 << 1,
    MiddleButton = 1 << 2,
    BackButton = 1 << 3,
    ExtraButton1 = 1 << 4,
    ExtraButton2 = 1 << 5,
    ExtraButton3 = 1 << 6,
    ExtraButton4 = 1 << 7
  };
  using MouseButton = MouseButtons;
  VME_ENUM_OPERATORS(MouseButtons)

  enum ModifierKeys
  {
    None = 0,
    Shift = 1 << 0,
    Ctrl = 1 << 1,
    Alt = 1 << 2,
  };
  VME_ENUM_OPERATORS(ModifierKeys)

  struct MouseEvent
  {
    MouseEvent(ScreenPosition pos, MouseButtons buttons, ModifierKeys modifiers) : _pos(pos), _buttons(buttons), _modifiers(modifiers) {}
    inline MouseButtons buttons() const noexcept
    {
      return _buttons;
    }

    inline ModifierKeys modifiers() const noexcept
    {
      return _modifiers;
    }

    inline ScreenPosition pos() const noexcept
    {
      return _pos;
    }

  private:
    ScreenPosition _pos;

    MouseButtons _buttons;
    ModifierKeys _modifiers;
  };
} // namespace VME

struct MouseAction
{
  struct RawItem
  {
    uint16_t serverId = 100;
    /*
      If true, the raw item is currently being drag in an area. Once released,
      each position of the area has an item of serverId added.
    */
    bool area = false;
  };

  struct Select
  {
    bool area = false;
  };

  struct None
  {
  };
};

/**
 * Utility class for sending UI information to a MapView.
 * 
 * This is a necessary effect of separating normal code and UI code
 *
 * */
class UIUtils
{
public:
  virtual ~UIUtils() = default;

  virtual ScreenPosition mouseScreenPosInView() = 0;
  virtual VME::ModifierKeys modifiers() const = 0;
};

using MouseAction_t = std::variant<MouseAction::None, MouseAction::RawItem, MouseAction::Select>;

/*
  Contains mouse actions that can occur on a MapView.
*/
class MapViewMouseAction
{
public:
  inline MouseAction_t action() const noexcept
  {
    return _mouseAction;
  };

  void set(const MouseAction_t action) noexcept
  {
    _mouseAction = action;
  }

  void setRawItem(uint16_t serverId) noexcept
  {
    _mouseAction = MouseAction::RawItem{serverId};
  }

  void reset() noexcept
  {
    _mouseAction = MouseAction::Select{};
  }

private:
  MouseAction_t _mouseAction = MouseAction::Select{};
};