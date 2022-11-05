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

    // ItemAttribute(ItemAttribute &&other) noexcept;
    // ItemAttribute &operator=(ItemAttribute &&other) noexcept;

    static std::string attributeTypeToString(const ItemAttribute_t attributeType);
    static std::optional<ItemAttribute_t> parseAttributeString(const std::string &attributeString);

    template <typename T>
    inline bool holds() const;

    template <typename T>
    inline std::optional<T> get() const;

    template <typename T>
    inline T as() const;

    bool operator==(const ItemAttribute &rhs) const;
    bool operator!=(const ItemAttribute &rhs) const;

    inline ItemAttribute_t type() const noexcept;
    inline ValueType value() const noexcept;

    void setBool(bool value);
    void setInt(int value);
    void setDouble(double value);
    void setString(const std::string &value);
    void setString(std::string &&value);

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

template <typename T>
inline bool ItemAttribute::holds() const
{
    return std::holds_alternative<T>(_value);
}

template <typename T>
inline std::optional<T> ItemAttribute::get() const
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

template <typename T>
inline T ItemAttribute::as() const
{
    return std::get<T>(_value);
}

template <typename T, typename... Ts>
inline std::ostream &operator<<(std::ostream &os, const std::variant<T, Ts...> &v)
{
    std::visit([&os](auto &&arg) { os << arg; }, v);
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const ItemAttribute &attribute)
{
    os << "{ ItemAttribute, value: " << attribute.value() << "}";
    return os;
}

inline std::ostream &operator<<(std::ostream &os, ItemAttribute_t type)
{
    os << ItemAttribute::attributeTypeToString(type);
    return os;
}
