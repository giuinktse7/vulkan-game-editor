#pragma once

#include <functional>
#include <ostream>

class Item;

enum class ContainerChangeType
{
    Insert,
    Remove,
    MoveInSameContainer
};

struct ContainerChange
{
    static ContainerChange inserted(uint8_t index);
    static ContainerChange removed(uint8_t index);
    static ContainerChange moveInSameContainer(uint8_t fromIndex, uint8_t toIndex);

    ContainerChangeType type;
    uint8_t index;

    // Only applicable for move in same container
    uint8_t toIndex;

  private:
    ContainerChange(ContainerChangeType type, uint8_t index);
    ContainerChange(ContainerChangeType type, uint8_t fromIndex, uint8_t toIndex);
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

inline std::ostream &operator<<(std::ostream &os, const ContainerChangeType &type)
{
    switch (type)
    {
        case ContainerChangeType::Insert:
            os << "ContainerChangeType::Insert";
            break;
        case ContainerChangeType::Remove:
            os << "ContainerChangeType::Remove";
            break;
        default:
            os << "Unknown ContainerChangeType";
            break;
    }

    return os;
}

inline std::ostream &operator<<(std::ostream &os, const ContainerChange &change)
{
    os << "{ type:" << change.type << ", index: " << static_cast<int>(change.index) << " }";
    return os;
}