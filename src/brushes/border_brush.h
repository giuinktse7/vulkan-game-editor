#pragma once

#include <array>
#include <limits>
#include <memory>
#include <optional>
#include <string>

#include "../tile_cover.h"
#include "border_brush_variation.h"
#include "brush.h"

struct Position;
class MapView;
class Map;
class GroundBrush;
class RawBrush;
class GeneralBorderBrush;
enum class BorderBrushVariationType;

class BorderBrush final : public Brush
{
  public:
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds);
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, GroundBrush *centerBrush);
    BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, RawBrush *centerBrush);

    void fixBorders(MapView &mapView, const Position &position, BorderNeighborMap &neighbors);

    void apply(MapView &mapView, const Position &position) override;
    void erase(MapView &mapView, const Position &position) override;

    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;
    const std::string getDisplayId() const override;
    std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const override;

    void apply(MapView &mapView, const Position &position, BorderType borderType);

    void quadrantChanged(MapView &mapView, const Position &position, TileQuadrant prevQuadrant, TileQuadrant currQuadrant);

    uint32_t iconServerId() const;

    void setIconServerId(uint32_t serverId);

    void setCenterGroundId(const std::string &id);

    uint32_t preferredZOrder() const;

    std::string brushId() const noexcept;

    bool includes(uint32_t serverId) const;
    bool hasBorder(uint32_t serverId) const;

    const std::vector<uint32_t> &serverIds() const;

    std::optional<uint32_t> getServerId(BorderType borderType) const noexcept;

    Brush *centerBrush() const;

    BorderType getBorderType(uint32_t serverId) const;
    TileCover getTileCover(uint32_t serverId) const;

    TileQuadrant getNeighborQuadrant(int dx, int dy);

    static void setBrushVariation(BorderBrushVariationType brushVariationType);

  private:
    friend class GeneralBorderBrush;
    friend class DetailedBorderBrush;

    void quadrantChanged(MapView &mapView, const Position &position, BorderNeighborMap &neighbors, TileQuadrant prevQuadrant, TileQuadrant currQuadrant);

    bool presentAt(const Map &map, const Position position) const;
    uint32_t borderItemAt(const Map &map, const Position position) const;

    void initialize();

    void updateCenter(BorderNeighborMap &neighbors);

    void fixBordersAtOffset(MapView &mapView, const Position &position, BorderNeighborMap &neighbors, int x, int y);
    void fixBorderEdgeCases(int x, int y, TileCover &cover, const BorderNeighborMap &neighbors);

    static void quadrantChangeInFirstTile(TileCover cover, TileCover &nextCover, TileQuadrant prevQuadrant, TileQuadrant currQuadrant);

    static TileBorderInfo tileInfo;

    static BorderExpandDirection currDir;
    static std::optional<BorderExpandDirection> prevDir;

    std::vector<uint32_t> sortedServerIds;

    std::string id;
    uint32_t _iconServerId;

    mutable uint32_t zOrder = std::numeric_limits<uint32_t>::max();

    BorderData borderData;

    static BorderBrushVariation *brushVariation;

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
};