#pragma once

#include <functional>

class Item;

struct ItemEntityIdDisconnect
{
    ItemEntityIdDisconnect();
    ItemEntityIdDisconnect(std::function<void()> f);

    ~ItemEntityIdDisconnect();

  private:
    std::function<void()> f;
};

struct TrackedItem
{
    TrackedItem(Item *item);

    Item *item() const noexcept;

    template <auto MemberFunction, typename T>
    void onChanged(T *instance)
    {
        onChangeCallback = std::bind(MemberFunction, instance, std::placeholders::_1);
    }

    inline uint32_t entityId() const noexcept;

  private:
    void updateItem(Item *item);

    uint32_t _entityId;
    Item *_item;

    ItemEntityIdDisconnect disconnect;
    std::function<void(Item *)> onChangeCallback;
};

inline uint32_t TrackedItem::entityId() const noexcept
{
    return _entityId;
}