#pragma once

#include <type_traits>

namespace VME
{
#define VME_ENUM_OPERATORS(EnumType)                                          \
  inline constexpr EnumType operator&(EnumType l, EnumType r)                 \
  {                                                                           \
    typedef std::underlying_type<EnumType>::type ut;                          \
    return static_cast<EnumType>(static_cast<ut>(l) & static_cast<ut>(r));    \
  }                                                                           \
                                                                              \
  inline constexpr EnumType operator|(EnumType l, EnumType r)                 \
  {                                                                           \
    typedef std::underlying_type<EnumType>::type ut;                          \
    return static_cast<EnumType>(static_cast<ut>(l) | static_cast<ut>(r));    \
  }                                                                           \
                                                                              \
  inline constexpr EnumType &operator&=(EnumType &lhs, const EnumType rhs)    \
  {                                                                           \
    typedef std::underlying_type<EnumType>::type ut;                          \
    lhs = static_cast<EnumType>(static_cast<ut>(lhs) & static_cast<ut>(rhs)); \
    return lhs;                                                               \
  }                                                                           \
                                                                              \
  inline constexpr EnumType &operator|=(EnumType &lhs, const EnumType rhs)    \
  {                                                                           \
    typedef std::underlying_type<EnumType>::type ut;                          \
    lhs = static_cast<EnumType>(static_cast<ut>(lhs) | static_cast<ut>(rhs)); \
    return lhs;                                                               \
  }

  enum MouseButtons
  {
    NoButton = 0,
    LeftButton = 1 << 0,
    RightButton = 1 << 1
  };
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
    MouseEvent(MouseButtons buttons, ModifierKeys modifiers) : _buttons(buttons), _modifiers(modifiers) {}
    inline MouseButtons buttons() const noexcept
    {
      return _buttons;
    }

    inline ModifierKeys modifiers() const noexcept
    {
      return _modifiers;
    }

  private:
    MouseButtons _buttons;
    ModifierKeys _modifiers;
  };

} // namespace VME

struct MouseAction
{
  struct RawItem
  {
    uint16_t serverId;
  };

  struct None
  {
  };
};

using MouseAction_t = std::variant<MouseAction::None, MouseAction::RawItem>;

/*
  Contains mouse actions that can occur on a MapView.
*/
class MapViewMouseAction
{
public:
  inline MouseAction_t action() const
  {
    return _mouseAction;
  };

  void set(const MouseAction_t action)
  {
    _mouseAction = action;
  }

  void reset()
  {
    _mouseAction = MouseAction::None{};
  }

private:
  MouseAction_t _mouseAction = MouseAction::None{};
};