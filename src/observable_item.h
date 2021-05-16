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

enum class ItemChangeType
{
    Count,
    ActionId,
    UniqueId
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

struct ItemGuidDisconnect
{
    ItemGuidDisconnect();
    ItemGuidDisconnect(std::function<void()> f);

    ~ItemGuidDisconnect();

  private:
    std::function<void()> f;
};

struct ObservableItem
{
    ObservableItem(Item *item);

    Item *item() const noexcept;

    template <auto MemberFunction, typename T>
    void onAddressChanged(T *instance)
    {
        onAddressChangedCallback = std::bind(MemberFunction, instance, std::placeholders::_1);
    }

    template <auto MemberFunction, typename T>
    void onPropertyChanged(T *instance)
    {
        onPropertyChangedCallback = std::bind(MemberFunction, instance, std::placeholders::_1, std::placeholders::_2);
    }

    void onPropertyChanged(std::function<void(Item *, ItemChangeType)> f)
    {
        onPropertyChangedCallback = f;
    }

    inline uint32_t guid() const noexcept;

  protected:
    void itemAddressChanged(Item *item);
    void itemPropertyChanged(ItemChangeType changeType);

    uint32_t _guid;
    Item *_item;

    ItemGuidDisconnect disconnect;
    std::function<void(Item *)> onAddressChangedCallback;
    std::function<void(Item *, ItemChangeType)> onPropertyChangedCallback;
};

struct TrackedContainer : ObservableItem
{
    TrackedContainer(Item *item);

    template <auto MemberFunction, typename T>
    void onContainerChanged(T *instance)
    {
        onContainerChangeCallback = std::bind(MemberFunction, instance, std::placeholders::_1);
    }

  private:
    void updateContainer(ContainerChange change);
    ItemGuidDisconnect containerDisconnect;
    std::function<void(ContainerChange)> onContainerChangeCallback;
};

inline uint32_t ObservableItem::guid() const noexcept
{
    return _guid;
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
        case ContainerChangeType::MoveInSameContainer:
            os << "ContainerChangeType::MoveInSameContainer";
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