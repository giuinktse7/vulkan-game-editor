#include "item_attribute.h"

ItemAttribute::ItemAttribute(ItemAttribute_t type)
    : type(type)
{
  switch (type)
  {
  case ItemAttribute_t::UniqueId:
  case ItemAttribute_t::ActionId:
    value = 0;
    break;
  case ItemAttribute_t::Text:
  case ItemAttribute_t::Description:
    value.emplace<std::string>("");
    break;
  }
}

void ItemAttribute::setBool(bool value)
{
  if (std::holds_alternative<bool>(this->value))
  {
    this->value = value;
  }
  else
  {
    Logger::error() << "Tried to assign value " << value << " to an ItemAttribute of type " << this->type;
  }
}

void ItemAttribute::setInt(int value)
{
  if (std::holds_alternative<int>(this->value))
  {
    this->value = value;
  }
  else
  {
    Logger::error() << "Tried to assign value " << value << " to an ItemAttribute of type " << this->type;
  }
}

void ItemAttribute::setDouble(double value)
{
  if (std::holds_alternative<double>(this->value))
  {
    this->value = value;
  }
  else
  {
    Logger::error() << "Tried to assign value " << value << " to an ItemAttribute of type " << this->type;
  }
}

void ItemAttribute::setString(std::string &value)
{
  if (std::holds_alternative<std::string>(this->value))
  {
    this->value.emplace<std::string>(value);
  }
  else
  {
    Logger::error() << "Tried to assign value " << value << " to an ItemAttribute of type " << this->type;
  }
}