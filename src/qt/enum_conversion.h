/*
This header file contains utility methods for converting Qt enums to VME enums.
All of this conversion code is a replacement for using many if's to perform the
conversion. This should reduce branching, resulting in faster conversion.
*/
#pragma once

#include <Qt>
#include <array>
#include <type_traits>

namespace VME
{
  enum ModifierKeys;
  enum MouseButtons;
} // namespace VME

namespace enum_conversion
{
  VME::ModifierKeys vmeModifierKeys(Qt::KeyboardModifiers modifiers);
  VME::MouseButtons vmeButtons(Qt::MouseButtons buttons);

  namespace detail
  {
    constexpr bool exactlyOneBitSet(const uint32_t bits)
    {
      return bits && !(bits & (bits - 1));
    }

    /**
      Utility for creating a conversion table to convert between bitflags. Used
      to convert QT bitflags to VME bitflags.
      
      The underlying value 0 is interpreted as "no flags".
      
      Example usage:
      
      <!-- Table for converting Qt::KeyboardModifiers to VME::ModifierKeys -->
      using ModifierKeysConversion = BitFlagConversionTable<
                        Qt::KeyboardModifier,
                        VME::ModifierKeys,
                        util::msb(Qt::KeyboardModifier::GroupSwitchModifier)>;
      constexpr ModifierKeysConversion createModifierKeyTable()
      {
        using qtKey = Qt::KeyboardModifier;
        using vmeKey = VME::ModifierKeys;
        ModifierKeysConversion table;
        table.add(qtKey::ShiftModifier, vmeKey::Shift);
        table.add(qtKey::ControlModifier, vmeKey::Ctrl);
        table.add(qtKey::AltModifier, vmeKey::Alt);
    
        return table;
      }
      constexpr ModifierKeysConversion ModifierKeysConversionTable = createModifierKeyTable();

      <!-- Then, to perform the conversion: -->
        auto enumValue = static_cast<Qt::KeyboardModifier>(static_cast<Qt::KeyboardModifiers::Int>(modifiers));
        auto converted = ModifierKeysConversionTable.from(enumValue);
    */
    template <typename From, typename To, size_t size>
    struct BitFlagConversionTable
    {
      constexpr BitFlagConversionTable();
      To arr[size];

      constexpr void add(const From from, const To to) noexcept;
      constexpr To from(From from) const;
    };

    template <typename From, typename To, size_t size>
    constexpr BitFlagConversionTable<From, To, size>::BitFlagConversionTable() : arr() {}

    template <typename From, typename To, size_t size>
    constexpr void BitFlagConversionTable<From, To, size>::add(const From from, const To to) noexcept
    {
      if (!exactlyOneBitSet((int)from))
      {
        throw std::logic_error("Assertion failed!");
      }
      arr[util::lsb(from)] = to;
    }

    template <typename From, typename To, size_t size>
    constexpr To BitFlagConversionTable<From, To, size>::from(From from) const
    {
      auto f = static_cast<std::underlying_type_t<From>>(from);
      To to = static_cast<To>(0);
      while (f != static_cast<From>(0))
      {
        int pos = util::lsb(f);
        to |= static_cast<To>(arr[pos]);
        f &= ~(1 << pos);
      }

      return to;
    }
  } // namespace detail
} // namespace enum_conversion
