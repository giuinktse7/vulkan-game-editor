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

private:
    void updateItem(Item *item);

    Item *_item;

    ItemEntityIdDisconnect disconnect;
    std::function<void(Item *)> onChangeCallback;
};
