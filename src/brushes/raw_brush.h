#pragma once

#include "brush.h"

struct Position;
class ItemType;
class MapView;

class RawBrush : public Brush
{
  public:
    RawBrush(ItemType *itemType);

    // Uses the item name as brush name
    static RawBrush fromServerId(uint32_t serverId);

    void apply(MapView &mapView, const Position &position) const override;
    uint32_t iconServerId() const override;
    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;

    ItemType *itemType() const noexcept;
    uint32_t serverId() const noexcept;

  private:
    std::string name;

    ItemType *_itemType;
};