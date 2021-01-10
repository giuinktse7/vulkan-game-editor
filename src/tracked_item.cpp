#include "tracked_item.h"

#include "item.h"
#include "items.h"

class Item;

TrackedItem::TrackedItem(Item *item)
    : _entityId(item->getEntityId().value()),
      disconnect(Items::items.trackItem<&TrackedItem::updateItem>(_entityId, this)),
      _item(item) {}

Item *TrackedItem::item() const noexcept
{
    return _item;
}

void TrackedItem::updateItem(Item *item)
{
    _item = item;

    if (onChangeCallback)
    {
        onChangeCallback(_item);
    }
}

TrackedContainer::TrackedContainer(Item *item)
    : TrackedItem(item),
      containerDisconnect(Items::items.trackContainer<&TrackedContainer::updateContainer>(_entityId, this)) {}

void TrackedContainer::updateContainer(ContainerChange change)
{
    if (onContainerChangeCallback)
    {
        onContainerChangeCallback(change);
    }
}

ItemEntityIdDisconnect::ItemEntityIdDisconnect()
{
}

ItemEntityIdDisconnect::ItemEntityIdDisconnect(std::function<void()> f)
    : f(f)
{
}

ItemEntityIdDisconnect::~ItemEntityIdDisconnect()
{
    if (f)
    {
        f();
    }
}