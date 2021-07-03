#pragma once

#include <string>
#include <unordered_set>
#include <variant>

#include "brush.h"

struct Position;
class MapView;
class GroundBrush;
class RawBrush;

struct BorderData
{
    std::array<uint32_t, 12> borderIds;

    // Must always be a GroundBrush or a RawBrush
    Brush *centerBrush;
};

class BorderBrush final : public Brush
{
  public:
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds);
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, GroundBrush *centerBrush);
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, RawBrush *centerBrush);

    void apply(MapView &mapView, const Position &position, Direction direction) override;
    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;
    const std::string getDisplayId() const override;
    std::vector<ThingDrawInfo> getPreviewTextureInfo(Direction direction) const override;

    uint32_t iconServerId() const;

    void setIconServerId(uint32_t serverId);

    std::string brushId() const noexcept;

  private:
    void initialize();
    uint32_t sampleServerId();

    std::string id;
    uint32_t _iconServerId;

    BorderData borderData;
};