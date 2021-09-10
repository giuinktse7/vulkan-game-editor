#pragma once

#include <functional>
#include <optional>

template <typename T>
class LazyObject
{
  public:
    LazyObject(std::function<T()> initializer)
        : initializer(initializer) {}

    LazyObject(T value)
        : initializer{}, _value(value) {}

    _NODISCARD constexpr const T &value() const &
    {
        if (!_value.has_value())
        {
            _value = initializer();
        }

        return _value.value();
    }

    _NODISCARD constexpr T &value() &
    {
        if (!_value.has_value())
        {
            _value = initializer();
        }

        return _value.value();
    }

    _NODISCARD constexpr const T &value_unsafe() const &
    {
        return _value.value();
    }

    _NODISCARD constexpr T &value_unsafe() &
    {
        return _value.value();
    }

    constexpr explicit operator bool() const noexcept
    {
        return _value;
    }

    _NODISCARD constexpr bool hasValue() const noexcept
    {
        return _value.has_value();
    }

    _NODISCARD constexpr const T &&value() const &&
    {
        if (!_value.has_value())
        {
            _value = initializer();
        }

        return _value.value();
    }

    _NODISCARD constexpr T valueOr(T &&other) const &
    {
        return _value.value_or(std::forward<T>(other));
    }

  private:
    std::function<T()> initializer;

    mutable std::optional<T> _value;
};