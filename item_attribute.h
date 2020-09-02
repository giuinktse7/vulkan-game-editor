#pragma once

#include <string>
#include <variant>
#include <optional>

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
  ItemAttribute(ItemAttribute_t type);
  ItemAttribute_t type;

  template <typename T>
  bool holds() const
  {
    return std::holds_alternative<T>(value);
  }

  template <typename T>
  std::optional<T> get()
  {
    if (std::holds_alternative<T>(value))
    {
      return std::get<T>(value);
    }
    else
    {
      return {};
    }
  }

protected:
  void setBool(bool value);
  void setInt(int value);
  void setDouble(double value);
  void setString(std::string &value);

private:
  std::variant<bool, int, double, std::string> value;
};

template <typename T, typename... Ts>
inline std::ostream &operator<<(std::ostream &os, const std::variant<T, Ts...> &v)
{
  std::visit([&os](auto &&arg) { os << arg; }, v);
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
    os << "Unknown ItemAttribute";
    break;
  }

  return os;
}
