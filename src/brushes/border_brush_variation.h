#pragma once

#include <optional>

#include "../tile_cover.h"

class NeighborMap;
struct BorderExpandResult;
class NeighborMap;
class MapView;
struct Position;

struct BorderBrushVariation
{
    virtual TileCover quadrantChanged(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection currDir, TileQuadrant prevQuadrant, TileQuadrant currQuadrant) const = 0;

    virtual void expandCenter(NeighborMap &neighbors, TileBorderInfo &tileInfo, TileQuadrant tileQuadrant) const = 0;
    virtual void expandNorth(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const = 0;
    virtual void expandEast(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const = 0;
    virtual void expandSouth(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const = 0;
    virtual void expandWest(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const = 0;
};

struct GeneralBorderBrush : public BorderBrushVariation
{
    TileCover quadrantChanged(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection currDir, TileQuadrant prevQuadrant, TileQuadrant currQuadrant) const override;

    void expandCenter(NeighborMap &neighbors, TileBorderInfo &tileInfo, TileQuadrant tileQuadrant) const override;
    void expandNorth(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandEast(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandSouth(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandWest(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
};

struct DetailedBorderBrush : public BorderBrushVariation
{
    TileCover quadrantChanged(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection currDir, TileQuadrant prevQuadrant, TileQuadrant currQuadrant) const override;

    void expandCenter(NeighborMap &neighbors, TileBorderInfo &tileInfo, TileQuadrant tileQuadrant) const override;
    void expandNorth(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandEast(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandSouth(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandWest(NeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
};