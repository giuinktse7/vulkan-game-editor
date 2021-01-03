#include "tracked_item.h"

#include "item.h"
#include "items.h"

class Item;

TrackedItem::TrackedItem(Item *item)
    : disconnect(Items::items.trackItem<&TrackedItem::updateItem>(item->getEntityId().value(), this))
{
    _item = item;
}

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

ItemEntityIdDisconnect::ItemEntityIdDisconnect()
{
}

ItemEntityIdDisconnect::ItemEntityIdDisconnect(std::function<void()> f) : f(f)
{
}

ItemEntityIdDisconnect::~ItemEntityIdDisconnect()
{
    if (f)
    {
        f();
    }
}