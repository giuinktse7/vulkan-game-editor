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
        : _value(value) {}

    LazyObject(std::function<T()> initializer, std::function<void(T)> onFirstUse)
        : initializer(initializer), onFirstUse(onFirstUse) {}

    LazyObject(T value, std::function<void(T)> onFirstUse)
        : initializer{}, onFirstUse(onFirstUse), _value(value) {}

    void setOnFirstUse(std::function<void(T)> onFirstUse)
    {
        this->onFirstUse = onFirstUse;
    }

    _NODISCARD constexpr const T &value() const &
    {
        initialize();
        return _value.value();
    }

    _NODISCARD constexpr T &value() &
    {
        initialize();
        return _value.value();
    }

    _NODISCARD constexpr const T &value_unsafe() const &
    {
        if (!hasBeenUsed)
        {
            onFirstUse(_value.value());
            hasBeenUsed = true;
        }

        return _value.value();
    }

    _NODISCARD constexpr T &value_unsafe() &
    {
        if (!hasBeenUsed)
        {
            onFirstUse(_value.value());
            hasBeenUsed = true;
        }

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
        initialize();
        return _value.value();
    }

    _NODISCARD constexpr T valueOr(T &&other) const &
    {
        if (_value)
        {
            hasBeenUsed = true;
        }

        return _value.value_or(std::forward<T>(other));
    }

  private:
    void initialize() const noexcept
    {
        if (!hasBeenUsed)
        {
            hasBeenUsed = true;
            if (initializer)
            {
                _value = initializer();
                initializer = {};
            }
            if (onFirstUse)
            {
                onFirstUse(_value.value());
                onFirstUse = {};
            }
        }
    }
    mutable std::function<void(T)> onFirstUse;
    mutable std::function<T()> initializer;

    mutable bool hasBeenUsed = false;

    mutable std::optional<T> _value;
};