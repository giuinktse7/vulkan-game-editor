#pragma once

#undef slots
#include <nano_signal_slot.hpp>
#define slots Q_SLOTS

#define REGISTER_SIGNAL(_ret_, _name_, ...)                \
  private:                                                 \
    Nano::Signal<_ret_(__VA_ARGS__)> _name_##_signal;      \
                                                           \
  public:                                                  \
    template <auto MemberFunction, typename T>             \
    void on_##_name_(T *instance)                          \
    {                                                      \
        _name_##_signal.connect<MemberFunction>(instance); \
    }

template <class T>
struct observable_unique_ptr
{
    REGISTER_SIGNAL(void, moved_or_destroyed)

    observable_unique_ptr(std::unique_ptr<T> &&pointer)
        : pointer(std::move(pointer)) {}
    observable_unique_ptr() noexcept {}

    observable_unique_ptr(observable_unique_ptr &&other) noexcept
        : pointer(std::move(other.pointer))
    {
        notifyDestroy();
    }

    observable_unique_ptr &operator=(observable_unique_ptr &&other) noexcept
    {
        notifyDestroy();

        pointer = std::move(other.pointer);

        return (*this);
    }

    ~observable_unique_ptr()
    {
        notifyDestroy();
    }

    std::unique_ptr<T> drop()
    {
        notifyDestroy();
        return std::move(pointer);
    }

    void reset() noexcept
    {
        pointer.reset();
        notifyDestroy();
    }

    void swap(std::unique_ptr<T> &&other)
    {
        std::swap(pointer, std::move(other));
        notifyDestroy();
    }

    _NODISCARD auto get() const noexcept
    {
        return pointer.get();
    }

    _NODISCARD auto operator*() const noexcept
    {
        return *pointer;
    }

    _NODISCARD auto operator->() const noexcept
    {
        return get();
    }

    explicit operator bool() const noexcept
    {
        return static_cast<bool>(pointer);
    }

    std::unique_ptr<T> pointer;

  private:
    void notifyDestroy()
    {
        moved_or_destroyed_signal.fire();
        moved_or_destroyed_signal.disconnect_all();
    }
};