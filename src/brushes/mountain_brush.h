#pragma once

#include <string>
#include <unordered_set>
#include <variant>

#include "../random.h"
#include "../tile.h"
#include "../tile_cover.h"
#include "brush.h"

struct Position;
class MapView;
class Tile;
class GroundBrush;
struct BorderZOrderBlock;
struct MountainGroundNeighborMap;

struct LazyGroundBrush
{
    LazyGroundBrush(RawBrush *brush);
    LazyGroundBrush(GroundBrush *brush);
    LazyGroundBrush(std::string groundBrushId);

    Brush *get() const;

    mutable bool evaluated = false;

  private:
    mutable std::variant<std::string, RawBrush *, GroundBrush *> data;
};

namespace MountainPart
{
    struct InnerWall
    {
        uint32_t east;
        uint32_t south;
        uint32_t southEast;

        bool contains(uint32_t serverId) const noexcept;
    };
} // namespace MountainPart

class MountainBrush final : public Brush
{
  private:
    static constexpr uint32_t DefaultZOrder = 900;

  public:
    MountainBrush(std::string id, std::string name, LazyGroundBrush ground, MountainPart::InnerWall innerWall, BorderData mountainBorders, uint32_t iconServerId);

    void borderize(MapView &mapView, MountainGroundNeighborMap &neighbors, Position pos, int dx, int dy);

    void apply(MapView &mapView, const Position &position) override;
    void erase(MapView &mapView, const Position &position) override;

    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;
    std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const override;
    const std::string getDisplayId() const override;

    uint32_t iconServerId() const;

    const std::string &id() const noexcept;

    const std::unordered_set<uint32_t> &serverIds() const;

    void setGround(RawBrush *brush);
    void setGround(GroundBrush *brush);
    void setGround(const std::string &groundBrushId);

    Brush *ground() const noexcept;

    uint32_t nextGroundServerId();

    TileCover getTileCover(uint32_t serverId) const;

    static bool isFeature(TileCover cover);

  private:
    void borderize(MapView &mapView, MountainGroundNeighborMap &neighbors, int dx, int dy);

    void initialize();

    void apply(MapView &mapView, const Position &position, BorderType borderType);

    void postBorderizePass(MapView &mapView, const Position &position);

    BorderType getBorderType(uint32_t serverId) const;

    mutable std::unordered_set<uint32_t> _serverIds;

    std::string _id;
    uint32_t _iconServerId;

    uint32_t totalWeight = 0;

    uint32_t _zOrder = DefaultZOrder;

    MountainPart::InnerWall innerWall;
    BorderData mountainBorders;

    LazyGroundBrush _ground;

    static constexpr TileCover features =
        TILE_COVER_WEST | TILE_COVER_NORTH |
        TILE_COVER_NORTH_WEST |
        TILE_COVER_NORTH_WEST_CORNER | TILE_COVER_NORTH_EAST_CORNER | TILE_COVER_SOUTH_WEST_CORNER;
};

struct MountainGroundNeighborMap
{
    struct Entry
    {
        bool hasMountainGround = false;
    };

    using value_type = MountainGroundNeighborMap::Entry;

    MountainGroundNeighborMap(Brush *ground, const Position &position, const Map &map);

    value_type at(int x, int y) const;
    value_type &at(int x, int y);

    void set(int x, int y, value_type entry);

    TileCover getTileCover(int dx, int dy) const noexcept;

    Position position;

    // Must be a RAW brush or a ground brush
    Brush *ground;

  private:
    int index(int x, int y) const;

    std::array<value_type, 25> data;
};

struct MountainWallNeighborMap
{
    using value_type = TileCover;

    MountainWallNeighborMap(MountainBrush *mountainBrush, const Position &position, const Map &map);

    value_type at(int x, int y) const;
    value_type &at(int x, int y);

    void set(int x, int y, value_type cover);

    Position position;

  private:
    int index(int x, int y) const;

    std::array<value_type, 25> data;
};