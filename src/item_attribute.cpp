#include "item_attribute.h"

ItemAttribute::ItemAttribute(ItemAttribute_t type)
    : _type(type)
{
  switch (type)
  {
  case ItemAttribute_t::UniqueId:
  case ItemAttribute_t::ActionId:
    _value = 0;
    break;
  case ItemAttribute_t::Text:
  case ItemAttribute_t::Description:
    _value.emplace<std::string>("");
    break;
  }
}

void ItemAttribute::setBool(bool value)
{
  if (std::holds_alternative<bool>(_value))
  {
    _value = value;
  }
  else
  {
    Logger::error() << "Tried to assign value " << value << " to an ItemAttribute of type " << _type;
  }
}

void ItemAttribute::setInt(int value)
{
  if (std::holds_alternative<int>(_value))
  {
    _value = value;
  }
  else
  {
    Logger::error() << "Tried to assign value " << value << " to an ItemAttribute of type " << _type;
  }
}

void ItemAttribute::setDouble(double value)
{
  if (std::holds_alternative<double>(_value))
  {
    _value = value;
  }
  else
  {
    Logger::error() << "Tried to assign value " << value << " to an ItemAttribute of type " << _type;
  }
}

void ItemAttribute::setString(const std::string &value)
{
  if (std::holds_alternative<std::string>(_value))
  {
    _value.emplace<std::string>(value);
  }
  else
  {
    Logger::error() << "Tried to assign value " << value << " to an ItemAttribute of type " << _type;
  }
}

void ItemAttribute::setString(std::string &&value)
{
  if (std::holds_alternative<std::string>(_value))
  {
    _value.emplace<std::string>(std::move(value));
  }
  else
  {
    Logger::error() << "Tried to assign value " << value << " to an ItemAttribute of type " << _type;
  }
}

bool ItemAttribute::operator==(const ItemAttribute &rhs) const
{
  return _value == rhs._value;
}

bool ItemAttribute::operator!=(const ItemAttribute &rhs) const
{
  return !(_value == rhs._value);
}