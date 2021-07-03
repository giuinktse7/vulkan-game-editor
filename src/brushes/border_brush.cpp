#include "border_brush.h"

#include "ground_brush.h"
#include "../map_view.h"
#include "raw_brush.h"

BorderBrush::BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, GroundBrush *centerBrush)
    : Brush(name), id(id), borderData{borderIds, centerBrush}
{
}

BorderBrush::BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds)
    : Brush(name), id(id), borderData{borderIds, nullptr}
{
}

BorderBrush::BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, RawBrush *centerBrush)
    : Brush(name), id(id), borderData{borderIds, centerBrush}
{
}

void BorderBrush::apply(MapView &mapView, const Position &position, Direction direction)
{
    // TODO Implement the apply correctly
    mapView.addItem(position, borderData.borderIds[0]);
}

bool BorderBrush::erasesItem(uint32_t serverId) const
{
    const auto &ids = borderData.borderIds;
    auto found = std::find(ids.begin(), ids.end(), serverId);
    return found != ids.end() || (borderData.centerBrush && borderData.centerBrush->erasesItem(serverId));
}

BrushType BorderBrush::type() const
{
    return BrushType::Border;
}

const std::string BorderBrush::getDisplayId() const
{
    return id;
}

std::vector<ThingDrawInfo> BorderBrush::getPreviewTextureInfo(Direction direction) const
{
    // TODO improve preview
    return std::vector<ThingDrawInfo>{DrawItemType(borderData.borderIds.at(0), PositionConstants::Zero)};
}

uint32_t BorderBrush::iconServerId() const
{
    return _iconServerId;
}

void BorderBrush::setIconServerId(uint32_t serverId)
{
    _iconServerId = serverId;
}

std::string BorderBrush::brushId() const noexcept
{
    return id;
}
