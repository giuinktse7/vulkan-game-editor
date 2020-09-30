#pragma once

#include <type_traits>

namespace VME
{
  enum MouseButtons
  {
    NoButton = 0,
    LeftButton = 1 << 0,
    RightButton = 1 << 1
  };

  inline constexpr MouseButtons operator&(MouseButtons l, MouseButtons r)
  {
    typedef std::underlying_type<MouseButtons>::type ut;
    return static_cast<MouseButtons>(static_cast<ut>(l) & static_cast<ut>(r));
  }

  inline constexpr MouseButtons operator|(MouseButtons l, MouseButtons r)
  {
    typedef std::underlying_type<MouseButtons>::type ut;
    return static_cast<MouseButtons>(static_cast<ut>(l) | static_cast<ut>(r));
  }

  inline constexpr MouseButtons &operator&=(MouseButtons &lhs, const MouseButtons rhs)
  {
    typedef std::underlying_type<MouseButtons>::type ut;
    lhs = static_cast<MouseButtons>(static_cast<ut>(lhs) & static_cast<ut>(rhs));
    return lhs;
  }

  inline constexpr MouseButtons &operator|=(MouseButtons &lhs, const MouseButtons rhs)
  {
    typedef std::underlying_type<MouseButtons>::type ut;
    lhs = static_cast<MouseButtons>(static_cast<ut>(lhs) | static_cast<ut>(rhs));
    return lhs;
  }

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