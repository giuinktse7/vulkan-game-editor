#include "wall_brush.h"

#include "../items.h"
#include "../map_view.h"

WallBrush::WallBrush(std::string id, const std::string &name, StraightPart &&horizontal, StraightPart &&vertical, Part &&corner, Part &&pole)
    : Brush(name), id(id), horizontal(std::move(horizontal)), vertical(std::move(vertical)), corner(std::move(corner)), pole(std::move(pole))
{
#define add_to_ids(xs)    \
    for (auto &item : xs) \
        _serverIds.emplace(item.id);

    add_to_ids(this->horizontal.items);
    add_to_ids(this->horizontal.doors);
    add_to_ids(this->horizontal.windows);

    add_to_ids(this->vertical.items);
    add_to_ids(this->vertical.doors);
    add_to_ids(this->vertical.windows);

void WallBrush::erase(MapView &mapView, const Position &position, Direction direction)
}

void WallBrush::apply(MapView &mapView, const Position &position, Direction direction)
{
    // TODO Place correct wall
    Tile &tile = mapView.getOrCreateTile(position);

    mapView.setBottomItem(position, _iconServerId);
}

std::vector<ThingDrawInfo> WallBrush::getPreviewTextureInfo(Direction direction) const
{
    return std::vector<ThingDrawInfo>{DrawItemType(_iconServerId, PositionConstants::Zero)};
}

const std::string WallBrush::getDisplayId() const
{
    return id;
}

bool WallBrush::erasesItem(uint32_t serverId) const
{
    return _serverIds.contains(serverId);
}

BrushType WallBrush::type() const
{
    return BrushType::Wall;
}

std::string WallBrush::brushId() const noexcept
{
    return id;
}

void WallBrush::finalize()
{
    for (auto serverId : _serverIds)
    {
        ItemType *itemType = Items::items.getItemTypeByServerId(serverId);
        if (itemType)
        {
            itemType->brush = this;
        }
    }
}

void WallBrush::setIconServerId(uint32_t serverId)
{
    _iconServerId = serverId;
}

uint32_t WallBrush::iconServerId() const
{
    return _iconServerId;
}