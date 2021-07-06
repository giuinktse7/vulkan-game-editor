#pragma once

#include <array>
#include <optional>
#include <string>

#include "../tile_cover.h"
#include "brush.h"

struct Position;
class MapView;
class Map;
class GroundBrush;
class RawBrush;

enum class BorderExpandDirection
{
    North,
    East,
    South,
    West,
    None
};

struct ExpandedTileBlock
{
    int x = 0;
    int y = 0;
    TileCover originalCover = TILE_COVER_NONE;
};

struct NeighborMap
{

    TileCover at(int x, int y) const;
    TileCover &at(int x, int y);
    void set(int x, int y, TileCover tileCover);

    bool isExpanded(int x, int y) const;
    bool hasExpandedCover() const noexcept;
    void addExpandedCover(int x, int y, TileCover originalCover);

    std::vector<ExpandedTileBlock> expandedCovers;

  private:
    int index(int x, int y) const;

    std::array<TileCover, 25> data;
};

struct BorderData
{
    BorderData(std::array<uint32_t, 12> borderIds, Brush *centerBrush)
        : borderIds(borderIds), centerBrush(centerBrush) {}

    // Must always be a GroundBrush or a RawBrush
    Brush *centerBrush;

    bool is(uint32_t serverId, BorderType borderType) const;
    uint32_t getServerId(BorderType borderType) const;
    BorderType getBorderType(uint32_t serverId) const;
    std::array<uint32_t, 12> getBorderIds() const;

  private:
    std::array<uint32_t, 12> borderIds;
};

class BorderBrush final : public Brush
{
  public:
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds);
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, GroundBrush *centerBrush);
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, RawBrush *centerBrush);

    void fixBorders(MapView &mapView, const Position &position, NeighborMap &neighbors);

    void apply(MapView &mapView, const Position &position, Direction direction) override;
    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;
    const std::string getDisplayId() const override;
    std::vector<ThingDrawInfo> getPreviewTextureInfo(Direction direction) const override;

    uint32_t iconServerId() const;

    void setIconServerId(uint32_t serverId);

    std::string brushId() const noexcept;

    bool includes(uint32_t serverId) const;

    const std::vector<uint32_t> &serverIds() const;

    Brush *centerBrush() const;

    BorderType getBorderType(uint32_t serverId) const;
    TileCover getTileCover(uint32_t serverId) const;

  private:
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, Brush *centerBrush);

    void apply(MapView &mapView, const Position &position, BorderType borderType);

    bool presentAt(const Map &map, const Position position) const;
    uint32_t borderItemAt(const Map &map, const Position position) const;
    TileCover getTileCoverAt(const Map &map, const Position position) const;

    void initialize();

    void expandCenter(NeighborMap &neighbors, TileQuadrant tileQuadrant);
    void expandNorth(NeighborMap &neighbors);
    void expandSouth(NeighborMap &neighbors);
    void expandWest(NeighborMap &neighbors);

    void fixBordersAtOffset(MapView &mapView, const Position &position, NeighborMap &neighbors, int x, int y);
    void fixBorderEdgeCases(int x, int y, TileCover &cover, const NeighborMap &neighbors);

    TileQuadrant getQuadrant(int dx, int dy);

    static TileQuadrant currQuadrant;
    static std::optional<TileQuadrant> previousQuadrant;

    static BorderExpandDirection currDir;
    static std::optional<BorderExpandDirection> prevDir;

    std::vector<uint32_t> sortedServerIds;

    std::string id;
    uint32_t _iconServerId;

    BorderData borderData;

    /*
    None = 0,
    North = 1,
    East = 2,
    South = 3,
    West = 4,
    NorthWestCorner = 5,
    NorthEastCorner = 6,
    SouthEastCorner = 7,
    SouthWestCorner = 8,
    NorthWestDiagonal = 9,
    NorthEastDiagonal = 10,
    SouthEastDiagonal = 11,
    SouthWestDiagonal = 12,
    Center = 13
*/
    static constexpr std::array<TileCover, 14> borderTypeToTileCover = {
        TILE_COVER_NONE,
        TILE_COVER_NORTH,
        TILE_COVER_EAST,
        TILE_COVER_SOUTH,
        TILE_COVER_WEST,
        TILE_COVER_NORTH_WEST_CORNER,
        TILE_COVER_NORTH_EAST_CORNER,
        TILE_COVER_SOUTH_EAST_CORNER,
        TILE_COVER_SOUTH_WEST_CORNER,
        TILE_COVER_NORTH_WEST,
        TILE_COVER_NORTH_EAST,
        TILE_COVER_SOUTH_EAST,
        TILE_COVER_SOUTH_WEST,
        TILE_COVER_FULL};
};
