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
