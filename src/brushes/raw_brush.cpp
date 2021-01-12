#include "raw_brush.h"

#include "../debug.h"
#include "../item_type.h"
#include "../items.h"

RawBrush::RawBrush(ItemType *itemType)
    : Brush(itemType->name), _itemType(itemType)
{
    DEBUG_ASSERT(_itemType != nullptr, "Invalid itemType. No ItemType for server ID " + itemType->id);
}

RawBrush RawBrush::fromServerId(uint32_t serverId)
{
    return RawBrush(Items::items.getItemTypeByServerId(serverId));
}

ItemType *RawBrush::itemType() const noexcept
{
    return _itemType;
}

uint32_t RawBrush::serverId() const noexcept
{
    return _itemType->id;
}

uint32_t RawBrush::iconServerId() const
{
    return _itemType->id;
}

bool RawBrush::erasesItem(uint32_t serverId) const
{
    return serverId == _itemType->id;
}

BrushType RawBrush::type() const
{
    return BrushType::RawItem;
}