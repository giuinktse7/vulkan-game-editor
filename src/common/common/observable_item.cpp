#include "observable_item.h"

#include "item.h"
#include "items.h"

class Item;

ObservableItem::ObservableItem(Item *item)
    : _guid(item->guid()),
      _item(item),
      disconnect(Items::items.trackItem<&ObservableItem::itemAddressChanged, &ObservableItem::itemPropertyChanged>(_guid, this)) {}

Item *ObservableItem::item() const noexcept
{
    return _item;
}

void ObservableItem::itemAddressChanged(Item *item)
{
    _item = item;

    if (onAddressChangedCallback)
    {
        onAddressChangedCallback(_item);
    }
}

void ObservableItem::itemPropertyChanged(ItemChangeType changeType)
{
    if (onPropertyChangedCallback)
    {
        onPropertyChangedCallback(_item, changeType);
    }
}

TrackedContainer::TrackedContainer(Item *item)
    : ObservableItem(item),
      containerDisconnect(Items::items.trackContainer<&TrackedContainer::updateContainer>(_guid, this)) {}

void TrackedContainer::updateContainer(ContainerChange change)
{
    if (onContainerChangeCallback)
    {
        onContainerChangeCallback(change);
    }
}

ContainerChange::ContainerChange(ContainerChangeType type, uint8_t index)
    : type(type), index(index) {}

ContainerChange::ContainerChange(ContainerChangeType type, uint8_t fromIndex, uint8_t toIndex)
    : type(type), index(fromIndex), toIndex(toIndex) {}

ContainerChange ContainerChange::inserted(uint8_t index)
{
    return ContainerChange(ContainerChangeType::Insert, index);
}

ContainerChange ContainerChange::removed(uint8_t index)
{
    return ContainerChange(ContainerChangeType::Remove, index);
}

ContainerChange ContainerChange::moveInSameContainer(uint8_t fromIndex, uint8_t toIndex)
{
    return ContainerChange(ContainerChangeType::MoveInSameContainer, fromIndex, toIndex);
}

ItemGuidDisconnect::ItemGuidDisconnect()
{
}

ItemGuidDisconnect::ItemGuidDisconnect(std::function<void()> f)
    : f(f)
{
}

ItemGuidDisconnect::~ItemGuidDisconnect()
{
    if (f)
    {
        f();
    }
}