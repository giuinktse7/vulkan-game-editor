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
    CreatureBrush(std::string name, uint32_t looktype);

    void apply(MapView &mapView, const Position &position) override;

    BrushResource brushResource() const override;

    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;

    ItemType *itemType() const noexcept;
    uint32_t serverId() const noexcept;

    std::vector<ThingDrawInfo> getPreviewTextureInfo() const override;
    const std::string getDisplayId() const override;

  private:
    const CreatureType *const creatureType;
};