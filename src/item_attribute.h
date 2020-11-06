#pragma once

#include <optional>
#include <string>
#include <variant>

#include "logger.h"
#include "util.h"

enum class ItemAttribute_t
{
  UniqueId = 1,
  ActionId = 2,
  Text = 3,
  Description = 4
};

class ItemAttribute
{
public:
  using ValueType = std::variant<bool, int, double, std::string>;
  ItemAttribute(ItemAttribute_t type);

  template <typename T>
  bool holds() const
  {
    return std::holds_alternative<T>(_value);
  }

  template <typename T>
  std::optional<T> get()
  {
    if (std::holds_alternative<T>(_value))
    {
      return std::get<T>(_value);
    }
    else
    {
      return {};
    }
  }

  bool operator==(const ItemAttribute &rhs) const
  {
    return _value == rhs._value;
  }

  inline ItemAttribute_t type() const noexcept;
  inline ValueType value() const noexcept;

  void setBool(bool value);
  void setInt(int value);
  void setDouble(double value);
  void setString(const std::string &value);
  void setString(std::string &&value);

protected:
private:
  ItemAttribute_t _type;

  ValueType _value;
};

inline ItemAttribute_t ItemAttribute::type() const noexcept
{
  return _type;
}

inline ItemAttribute::ValueType ItemAttribute::value() const noexcept
{
  return _value;
}

template <typename T, typename... Ts>
inline std::ostream &operator<<(std::ostream &os, const std::variant<T, Ts...> &v)
{
  std::visit([&os](auto &&arg) { os << arg; }, v);
  return os;
}

inline std::ostream &operator<<(std::ostream &os, const ItemAttribute &attribute)
{
  os << "{ type: " << attribute.type() << ", value: " << attribute.value() << "}";
  return os;
}

inline std::ostream &operator<<(std::ostream &os, ItemAttribute_t type)
{
  switch (type)
  {
  case ItemAttribute_t::UniqueId:
    os << "UniqueId";
    break;
  case ItemAttribute_t::ActionId:
    os << "ActionId";
    break;
  case ItemAttribute_t::Text:
    os << "Text";
    break;
  case ItemAttribute_t::Description:
    os << "Description";
    break;
  default:
    Logger::error() << "Could not convert ItemAttribute_t '" << to_underlying(type) << "' to a string.";
    os << to_underlying(type);
    break;
  }

  return os;
}
