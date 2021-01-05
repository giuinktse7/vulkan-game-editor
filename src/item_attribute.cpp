#include "item_attribute.h"

namespace
{
    vme_unordered_map<ItemAttribute_t, std::string> attributeToStringMap = {
        std::make_pair(ItemAttribute_t::UniqueId, "UniqueId"),
        std::make_pair(ItemAttribute_t::ActionId, "ActionId"),
        std::make_pair(ItemAttribute_t::Text, "Text"),
        std::make_pair(ItemAttribute_t::Description, "Description"),
    };
    vme_unordered_map<std::string, ItemAttribute_t> stringToAttributeMap = {
        std::make_pair("UniqueId", ItemAttribute_t::UniqueId),
        std::make_pair("ActionId", ItemAttribute_t::ActionId),
        std::make_pair("Text", ItemAttribute_t::Text),
        std::make_pair("Description", ItemAttribute_t::Description),
    };
    ;
} // namespace

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

// ItemAttribute::ItemAttribute(ItemAttribute &&other) noexcept
//     : _type(std::move(other._type)), _value(std::move(other._value)) {}

// ItemAttribute &ItemAttribute::operator=(ItemAttribute &&other) noexcept
// {
//   _type = std::move(other._type);
//   _value = std::move(other._value);

//   return *this;
// }

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

std::string ItemAttribute::attributeTypeToString(const ItemAttribute_t attributeType)
{
    return attributeToStringMap.at(attributeType);
}

std::optional<ItemAttribute_t> ItemAttribute::parseAttributeString(const std::string &attributeString)
{
    auto found = stringToAttributeMap.find(attributeString);
    return found != stringToAttributeMap.end()
               ? std::optional<ItemAttribute_t>(found->second)
               : std::nullopt;
}

bool ItemAttribute::operator==(const ItemAttribute &rhs) const
{
    return _value == rhs._value;
}

bool ItemAttribute::operator!=(const ItemAttribute &rhs) const
{
    return !(_value == rhs._value);
}
