#include "raw_brush.h"

#include "../debug.h"
#include "../item_type.h"
#include "../items.h"
#include "../map_view.h"
#include "../position.h"
#include "../tile.h"

RawBrush::RawBrush(ItemType *itemType)
    : Brush(itemType->name()), _itemType(itemType)
{
    DEBUG_ASSERT(_itemType != nullptr, "Invalid itemType. No ItemType for server ID " + std::to_string(itemType->id));

    _brushResource.id = itemType->id;
    _brushResource.type = BrushResourceType::ItemType;
    _brushResource.variant = 0;
}

void RawBrush::apply(MapView &mapView, const Position &position, Direction direction)
{
    if (!_itemType)
    {
        VME_LOG_ERROR("Attempted to draw with a RawBrush with a NULL ItemType (brush name: " << _name << ").");
        return;
    }

    Tile &tile = mapView.getOrCreateTile(position);

    if (_itemType->stackOrder == TileStackOrder::Bottom && tile.hasItems())
    {
        mapView.removeItems(tile, [](const Item &item) { return item.isBottom(); });
    }

    mapView.addItem(position, Item(_itemType->id));
}

BrushResource RawBrush::brushResource() const
{
    return _brushResource;
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
    return BrushType::Raw;
}

std::vector<ThingDrawInfo> RawBrush::getPreviewTextureInfo(Direction direction) const
{
    return std::vector<ThingDrawInfo>{DrawItemType(_itemType, PositionConstants::Zero)};
}

const std::string RawBrush::getDisplayId() const
{
    return std::to_string(_itemType->id);
}