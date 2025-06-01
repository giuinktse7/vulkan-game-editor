#pragma once

#include "../tile_cover.h"

struct BorderNeighborMap;
struct BorderExpandResult;
class MapView;
struct Position;

struct BorderBrushVariation
{
    virtual TileCover quadrantChanged(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection currDir, TileQuadrant prevQuadrant, TileQuadrant currQuadrant) const = 0;

    virtual void expandCenter(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, TileQuadrant tileQuadrant) const = 0;
    virtual void expandNorth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const = 0;
    virtual void expandEast(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const = 0;
    virtual void expandSouth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const = 0;
    virtual void expandWest(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const = 0;
};

struct GeneralBorderBrush : public BorderBrushVariation
{
    static GeneralBorderBrush instance;
    TileCover quadrantChanged(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection currDir, TileQuadrant prevQuadrant, TileQuadrant currQuadrant) const override;

    void expandCenter(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, TileQuadrant tileQuadrant) const override;
    void expandNorth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandEast(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandSouth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandWest(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
};

struct DetailedBorderBrush : public BorderBrushVariation
{
    static DetailedBorderBrush instance;

    TileCover quadrantChanged(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection currDir, TileQuadrant prevQuadrant, TileQuadrant currQuadrant) const override;

    void expandCenter(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, TileQuadrant tileQuadrant) const override;
    void expandNorth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandEast(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandSouth(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
    void expandWest(BorderNeighborMap &neighbors, TileBorderInfo &tileInfo, BorderExpandDirection prevDir) const override;
};