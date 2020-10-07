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

  struct Pan
  {
    std::optional<WorldPosition> cameraOrigin;
    std::optional<ScreenPosition> mouseOrigin;

    inline bool active() const
    {
      return cameraOrigin && mouseOrigin;
    }

    void stop()
    {
      cameraOrigin.reset();
      mouseOrigin.reset();
    }
  };

  struct None
  {
  };
};

using MouseAction_t = std::variant<MouseAction::None, MouseAction::RawItem, MouseAction::Select, MouseAction::Pan>;

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

/*
  Contains mouse actions that can occur on a MapView.
*/
class EditorAction
{
public:
  inline MouseAction_t &action() noexcept
  {
    return _action;
  };

  inline void setPrevious() noexcept
  {
    _action = _previousAction;
    _previousAction = MouseAction::None{};
  }

  inline MouseAction_t previous() const noexcept
  {
    return _previousAction;
  }

  void set(const MouseAction_t action) noexcept
  {
    _previousAction = _action;
    _action = action;
  }

  void setRawItem(uint16_t serverId) noexcept
  {
    set(MouseAction::RawItem{serverId});
  }

  void reset() noexcept
  {
    set(MouseAction::Select{});
  }

  template <typename T>
  T *as()
  {
    return std::get_if<T>(&_action);
  }

  template <typename T>
  bool is()
  {
    return std::holds_alternative<T>(_action);
  }

private:
  MouseAction_t _previousAction = MouseAction::Select{};
  MouseAction_t _action = MouseAction::Select{};
};