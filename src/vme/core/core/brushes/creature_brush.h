#pragma once

#include <string>
#include <vector>

#include "brush.h"

struct Position;
class ItemType;
class MapView;
class CreatureType;
enum class Direction;

class CreatureBrush final : public Brush
{
  public:
    CreatureBrush(CreatureType *creatureType);

    void apply(MapView &mapView, const Position &position) override;
    void erase(MapView &mapView, const Position &position) override;

    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;

    ItemType *itemType() const noexcept;
    uint32_t serverId() const noexcept;

    const std::string &id() const noexcept;

    std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const override;
    std::string getDisplayId() const override;
    int variationCount() const override;

    const CreatureType *const creatureType;

  private:
};