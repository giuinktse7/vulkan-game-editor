#pragma once

#include <string>
#include <unordered_set>
#include <variant>

#include "../lazy_object.h"
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
struct MountainNeighborMap;

namespace MountainPart
{
    struct InnerWall
    {
        uint32_t east;
        uint32_t south;
        uint32_t southEast;

        bool contains(uint32_t serverId) const noexcept;
    };

    struct MountainCover
    {
        MountainCover(TileCover featureCover, TileCover borderCover, MountainBrush *brush);

        TileCover featureCover;
        TileCover borderCover;
        MountainBrush *brush;
    };

    struct BorderBlock
    {
        std::vector<MountainCover> covers = {};
        MountainBrush *ground = nullptr;

        /**
         * z-order of the ground mountain brush
         */
        uint32_t zOrder() const noexcept;

        const MountainCover *getCover(MountainBrush *brush) const;

        void add(TileCover cover, MountainBrush *brush);
        void add(TileCover featureCover, TileCover borderCover, MountainBrush *brush);
        void addFeature(TileCover cover, MountainBrush *brush);
        void addBorder(TileCover cover, MountainBrush *brush);

        void add(const MountainCover &block);
        void merge(const BorderBlock &other);

        void sort();
    };
} // namespace MountainPart

class MountainBrush final : public Brush
{
  private:
    static constexpr uint32_t DefaultZOrder = 900;

  public:
    MountainBrush(std::string id, std::string name, Brush::LazyGround ground, MountainPart::InnerWall innerWall, BorderData mountainBorders, uint32_t iconServerId);
    MountainBrush(std::string id, std::string name, Brush::LazyGround ground, MountainPart::InnerWall innerWall, BorderData mountainBorders, std::string outerBorderBrushId, uint32_t iconServerId);

    static void generalBorderize(MapView &mapView, const Position &position);

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
    TileCover getBorderTileCover(uint32_t serverId) const;

    static bool isFeature(TileCover cover);

    void setOuterBorder(std::string outerBorderId);

    void borderize(MapView &mapView, const Position &position);

    void addServerId(uint32_t serverId);

    bool isFeature(const ItemType &itemType) const;
    bool isOuterBorder(const ItemType &itemType) const;

    static constexpr TileCover Features =
        TILE_COVER_WEST | TILE_COVER_NORTH |
        TILE_COVER_NORTH_WEST |
        TILE_COVER_NORTH_WEST_CORNER | TILE_COVER_NORTH_EAST_CORNER | TILE_COVER_SOUTH_WEST_CORNER;

  private:
    void borderize(MapView &mapView, MountainGroundNeighborMap &neighbors, int dx, int dy);

    void applyGround(MapView &mapView, const Position &position);

    void initialize();

    void apply(MapView &mapView, const Position &position, BorderType borderType);
    static void applyFeature(MapView &mapView, const Position &position, MountainBrush *brush, BorderType borderType);
    static void applyBorder(MapView &mapView, const Position &position, MountainBrush *brush, BorderType borderType);

    void postBorderizePass(MapView &mapView, const Position &position);

    BorderType getBorderType(uint32_t serverId) const;

    static void fixBorders(MapView &mapView, const Position &position, MountainNeighborMap &neighbors);
    static void fixBordersAtOffset(MapView &mapView, const Position &position, MountainNeighborMap &neighbors, int x, int y);

    mutable std::unordered_set<uint32_t> _serverIds;

    std::string _id;
    uint32_t _iconServerId;

    uint32_t totalWeight = 0;

    uint32_t _zOrder = DefaultZOrder;

    MountainPart::InnerWall innerWall;
    BorderData mountainBorder;

    std::optional<LazyObject<BorderBrush *>> outerBorder;

    Brush::LazyGround _ground;
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
    TileCover getBorderTileCover(int dx, int dy) const noexcept;

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

struct MountainNeighborMap
{
    using value_type = MountainPart::BorderBlock;
    MountainNeighborMap(const Position &position, const Map &map);
    value_type at(int x, int y) const;
    value_type &at(int x, int y);
    value_type &center();
    void set(int x, int y, value_type groundBorder);

    bool isExpanded(int x, int y) const;
    bool hasExpandedCover() const noexcept;
    void addExpandedCover(int x, int y);

    TileCover getExcludeMask(int dx, int dy);

    std::vector<ExpandedTileBlock> expandedCovers;

    static void addGroundBorder(value_type &self, const value_type &other, TileCover border);

    static void mirrorNorth(value_type &source, const value_type &borders);
    static void mirrorEast(value_type &source, const value_type &borders);
    static void mirrorSouth(value_type &source, const value_type &borders);
    static void mirrorWest(value_type &source, const value_type &borders);

    static void mirrorNorthWest(value_type &source, const value_type &borders);
    static void mirrorNorthEast(value_type &source, const value_type &borders);
    static void mirrorSouthEast(value_type &source, const value_type &borders);
    static void mirrorSouthWest(value_type &source, const value_type &borders);

    void addCenterCorners();

  private:
    int index(int x, int y) const;
    std::optional<MountainPart::BorderBlock> getTileCoverAt(const Map &map, const Position position) const;
    bool isMountainFeaturePart(const ItemType &itemType) const;
    bool isMountainOuterBorderPart(const ItemType &itemType) const;

    std::array<value_type, 25> data;
};