#pragma once

#include "brush.h"

struct Position;
class ItemType;
class MapView;

class RawBrush final : public Brush
{
  public:
    RawBrush(ItemType *itemType);

    // Uses the item name as brush name
    static RawBrush fromServerId(uint32_t serverId);

    void apply(MapView &mapView, const Position &position) override;
    void erase(MapView &mapView, const Position &position) override;

    uint32_t iconServerId() const;
    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;

    ItemType *itemType() const noexcept;
    uint32_t serverId() const noexcept;

    std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const override;

    const std::string getDisplayId() const override;

  private:
    ItemType *_itemType;
};