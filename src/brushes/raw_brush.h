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

    BrushResource brushResource() const override;

    void apply(MapView &mapView, const Position &position, Direction direction) override;
    uint32_t iconServerId() const;
    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;

    ItemType *itemType() const noexcept;
    uint32_t serverId() const noexcept;

    std::vector<ThingDrawInfo> getPreviewTextureInfo(Direction direction) const override;

    const std::string getDisplayId() const override;

  private:
    ItemType *_itemType;

    // Info that lets the GUI know how to draw the brush
    BrushResource _brushResource;
};