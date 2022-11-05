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
class BorderBrush;
struct BorderCover;
struct GroundNeighborMap;
class GroundBrush;

enum class BorderAlign
{
    Inner,
    Outer
};

struct TileBorderBlock
{
    std::vector<BorderCover> covers = {};
    GroundBrush *ground = nullptr;

    uint32_t zOrder() const noexcept;

    const BorderCover *border(BorderBrush *brush) const;

    void add(TileCover cover, BorderBrush *brush);

    void add(const BorderCover &block);
    void merge(const TileBorderBlock &other);

    void sort();
};

struct GroundBorder
{
    BorderBrush *brush;
    /**
     * LazyGround with nullptr corresponds to "none"
     */
    std::optional<Brush::LazyGround> to;
    BorderAlign align = BorderAlign::Outer;
};

/**
 * Ground Brush
 * 
 * Possible optimization:
 * nextServerId() performs linear search. It is the fastest approach for small lists (< 30 items at least).
 * However, if in the future it is common to use larger ground brushes (40-60+ items) then a possible optimization to try
 * is Walker's Alias Method:
 * https://gamedev.stackexchange.com/a/162996
 * C++ Implementation: https://gist.github.com/Liam0205/0b5786e9bfc73e75eb8180b5400cd1f8
 */

class GroundBrush final : public Brush
{
  private:
    static constexpr uint32_t DefaultZOrder = 900;

  public:
    GroundBrush(const std::string &id, std::vector<WeightedItemId> &&weightedIds, uint32_t zOrder = DefaultZOrder);
    GroundBrush(std::string id, const std::string &name, std::vector<WeightedItemId> &&weightedIds);
    GroundBrush(std::string id, const std::string &name, std::vector<WeightedItemId> &&weightedIds, uint32_t iconServerId);

    static void borderize(MapView &mapView, const Position &position);

    void apply(MapView &mapView, const Position &position) override;
    void applyWithoutBorderize(MapView &mapView, const Position &position) override;

    void erase(MapView &mapView, const Position &position) override;

    uint32_t iconServerId() const;

    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;

    void setIconServerId(uint32_t serverId);
    void setName(std::string name);

    uint32_t nextServerId() const;

    std::string brushId() const noexcept;

    std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const override;

    const std::string getDisplayId() const override;

    const std::unordered_set<uint32_t> &serverIds() const;

    static void restrictReplacement(const Tile *tile);
    static void disableReplacement();
    static void resetReplacementFilter();

    static bool hasReplacementFilter() noexcept;
    static bool mayPlaceOnTile(Tile &tile);

    void addBorder(GroundBorder border);

    uint32_t zOrder() const noexcept;

    BorderBrush *getBorderTowards(const GroundBrush *groundBrush, BorderAlign align) const;
    BorderBrush *getBorderTowards(Tile *tile, BorderAlign align) const;

    BorderBrush *findBorderTowards(const GroundBrush *groundBrush, BorderAlign align) const;

    BorderBrush *getDefaultBorderBrush(BorderAlign align) const;

    const std::vector<GroundBorder> &getBorders() const noexcept;

  private:
    void preBorderize(MapView &mapView, const Position &position, GroundNeighborMap &neighbors);
    void postBorderize(MapView &mapView, const Position &position, GroundNeighborMap &neighbors);
    static void apply(MapView &mapView, const Position &position, const BorderBrush *brush, BorderType borderType);

    static void fixBorders(MapView &mapView, const Position &position, GroundNeighborMap &neighbors);
    static void fixBordersAtOffset(MapView &mapView, const Position &position, GroundNeighborMap &neighbors, int x, int y);

    void initialize();
    uint32_t sampleServerId() const;

    std::unordered_set<uint32_t> _serverIds;
    std::vector<WeightedItemId> _weightedIds;

    std::vector<GroundBorder> borders;

    std::string id;
    uint32_t _iconServerId;

    uint32_t totalWeight = 0;

    uint32_t _zOrder = DefaultZOrder;

    /*
      std::monostate   -> May replace any ground
      uint32_t         -> May only replace that ground ID
      GroundBrush*     -> May replace any ground tile that is part of the brush
    */
    static std::variant<std::monostate, uint32_t, const GroundBrush *> replacementFilter;
};

struct GroundNeighborMap
{
    using value_type = TileBorderBlock;
    GroundNeighborMap(GroundBrush *centerGround, const Position &position, const Map &map);
    value_type at(int x, int y) const;
    value_type &at(int x, int y);
    value_type &center();
    void set(int x, int y, value_type groundBorder);

    bool isExpanded(int x, int y) const;
    bool hasExpandedCover() const noexcept;
    void addExpandedCover(int x, int y);

    TileCover getExcludeMask(int dx, int dy);

    std::vector<ExpandedTileBlock> expandedCovers;
    GroundBrush *centerGround;

    static void addBorderFromGround(value_type &self, const value_type &other, TileCover border);

    void mirrorNorth(value_type &source, int dx, int dy);
    void mirrorEast(value_type &source, const value_type &borders);
    void mirrorSouth(value_type &source, const value_type &borders);
    void mirrorWest(value_type &source, const value_type &borders);

    void mirrorNorthWest(value_type &source, const value_type &borders);
    void mirrorNorthEast(value_type &source, const value_type &borders);
    void mirrorSouthEast(value_type &source, const value_type &borders);
    void mirrorSouthWest(value_type &source, const value_type &borders);

    void addCenterCorners();

  private:
    int index(int x, int y) const;
    std::optional<value_type> getTileCoverAt(const Map &map, const Position position, TileCover mask) const;

    std::array<value_type, 25> data;

    // Used to remove adjacent borders when placing a new ground using GroundBrush
    static constexpr std::array<TileCover, 25> indexToBorderExclusionMask = {
        // Row 1
        TILE_COVER_NONE,
        TILE_COVER_NONE,
        TILE_COVER_NONE,
        TILE_COVER_NONE,
        TILE_COVER_NONE,

        // Row 2
        TILE_COVER_NONE,
        TILE_COVER_SOUTH_EAST_CORNER,
        TILE_COVER_SOUTH | TILE_COVER_SOUTH_EAST | TILE_COVER_SOUTH_WEST,
        TILE_COVER_SOUTH_WEST_CORNER,
        TILE_COVER_NONE,

        // Row 3
        TILE_COVER_NONE,
        TILE_COVER_EAST | TILE_COVER_NORTH_EAST | TILE_COVER_SOUTH_EAST,
        TILE_COVER_NONE,
        TILE_COVER_WEST | TILE_COVER_NORTH_WEST | TILE_COVER_SOUTH_WEST,
        TILE_COVER_NONE,

        // Row 4
        TILE_COVER_NONE,
        TILE_COVER_NORTH_EAST_CORNER,
        TILE_COVER_NORTH | TILE_COVER_NORTH_WEST | TILE_COVER_NORTH_EAST,
        TILE_COVER_NORTH_WEST_CORNER,
        TILE_COVER_NONE,

        // Row 5
        TILE_COVER_NONE,
        TILE_COVER_NONE,
        TILE_COVER_NONE,
        TILE_COVER_NONE,
        TILE_COVER_NONE,
    };
};