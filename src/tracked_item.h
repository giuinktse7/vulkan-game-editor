#pragma once

#include <functional>

class Item;

enum class ContainerChangeType
{
    Insert,
    Remove
};

struct ContainerChange
{
    static ContainerChange inserted(uint8_t index)
    {
        return ContainerChange(ContainerChangeType::Insert, index);
    }
    static ContainerChange removed(uint8_t index)
    {
        return ContainerChange(ContainerChangeType::Remove, index);
    }

    ContainerChangeType type;
    uint8_t index;

  private:
    ContainerChange(ContainerChangeType type, uint8_t index)
        : type(type), index(index) {}
};

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

  protected:
    void updateItem(Item *item);

    uint32_t _entityId;
    Item *_item;

    ItemEntityIdDisconnect disconnect;
    std::function<void(Item *)> onChangeCallback;
};

struct TrackedContainer : TrackedItem
{
    TrackedContainer(Item *item);

    template <auto MemberFunction, typename T>
    void onContainerChanged(T *instance)
    {
        onContainerChangeCallback = std::bind(MemberFunction, instance, std::placeholders::_1);
    }

  private:
    void updateContainer(ContainerChange change);
    ItemEntityIdDisconnect containerDisconnect;
    std::function<void(ContainerChange)> onContainerChangeCallback;
};

inline uint32_t TrackedItem::entityId() const noexcept
{
    return _entityId;
}