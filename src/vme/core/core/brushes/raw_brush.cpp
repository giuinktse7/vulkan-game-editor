#include "raw_brush.h"

#include "../debug.h"
#include "../item_type.h"
#include "../items.h"
#include "../map_view.h"
#include "../position.h"
#include "../settings.h"
#include "../tile.h"

RawBrush::RawBrush(ItemType *itemType)
    : Brush(itemType->name()), _itemType(itemType)
{
    DEBUG_ASSERT(_itemType != nullptr, "Invalid itemType. No ItemType for server ID " + std::to_string(itemType->id));
}

void RawBrush::erase(MapView &mapView, const Position &position)
{
    mapView.removeItems(position, [this](const Item &item) {
        return item.serverId() == this->serverId();
    });
}

void RawBrush::apply(MapView &mapView, const Position &position)
{
    if (!_itemType)
    {
        VME_LOG_ERROR("Attempted to draw with a RawBrush with a NULL ItemType (brush name: " << _name << ").");
        return;
    }

    Tile &tile = mapView.getOrCreateTile(position);

    if (_itemType->stackOrder == TileStackOrder::Bottom)
    {
        mapView.setBottomItem(tile, Item(_itemType->id));
    }
    else
    {
        mapView.addItem(tile, Item(_itemType->id), Settings::BRUSH_INSERTION_OFFSET);
    }
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

std::vector<ThingDrawInfo> RawBrush::getPreviewTextureInfo(int variation) const
{
    return std::vector<ThingDrawInfo>{DrawItemType(_itemType, PositionConstants::Zero)};
}

const std::string RawBrush::getDisplayId() const
{
    return std::to_string(_itemType->id);
}