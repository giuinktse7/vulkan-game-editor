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
    DEBUG_ASSERT(_itemType != nullptr, "Invalid itemType. No ItemType for server ID " + itemType->id);
}

void RawBrush::apply(MapView &mapView, const Position &position) const
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

    /*
    if(!itemtype) return;

	bool b = parameter? *reinterpret_cast<bool*>(parameter) : false;
	if((g_settings.getInteger(Config::RAW_LIKE_SIMONE) && !b) && itemtype->alwaysOnBottom && itemtype->alwaysOnTopOrder == 2) {
		for(ItemVector::iterator iter = tile->items.begin(); iter != tile->items.end();) {
			Item* item = *iter;
			if(item->getTopOrder() == itemtype->alwaysOnTopOrder) {
				delete item;
				iter = tile->items.erase(iter);
			}
			else
				++iter;
		}
	}
	tile->addItem(Item::Create(itemtype->id));
    */
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