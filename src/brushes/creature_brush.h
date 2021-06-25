#pragma once

#include <string>
#include <vector>

#include "brush.h"

struct Position;
class ItemType;
class MapView;
class CreatureType;

class CreatureBrush final : public Brush
{
  public:
    CreatureBrush(CreatureType *creatureType);

    void apply(MapView &mapView, const Position &position, Direction direction) override;

    BrushResource brushResource() const override;

    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;

    ItemType *itemType() const noexcept;
    uint32_t serverId() const noexcept;

    const std::string &id() const noexcept;

    std::vector<ThingDrawInfo> getPreviewTextureInfo(Direction direction) const override;
    const std::string getDisplayId() const override;

  private:
    const CreatureType *const creatureType;
};