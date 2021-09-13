#pragma once

#include <string>
#include <type_traits>

#include "const.h"

enum TileCover
{
    TILE_COVER_NONE = 0,
    TILE_COVER_FULL = 1,
    TILE_COVER_NORTH = 1 << 1,
    TILE_COVER_EAST = 1 << 2,
    TILE_COVER_SOUTH = 1 << 3,
    TILE_COVER_WEST = 1 << 4,
    TILE_COVER_NORTH_WEST = 1 << 5,
    TILE_COVER_NORTH_EAST = 1 << 6,
    TILE_COVER_SOUTH_WEST = 1 << 7,
    TILE_COVER_SOUTH_EAST = 1 << 8,
    TILE_COVER_NORTH_WEST_CORNER = 1 << 9,
    TILE_COVER_NORTH_EAST_CORNER = 1 << 10,
    TILE_COVER_SOUTH_WEST_CORNER = 1 << 11,
    TILE_COVER_SOUTH_EAST_CORNER = 1 << 12,
};

struct TileBorderInfo
{
    TileQuadrant prevQuadrant;
    TileQuadrant quadrant;
    TileQuadrant borderStartQuadrant;
    TileCover cover = TILE_COVER_NONE;
};

inline constexpr TileCover operator~(const TileCover &lhs)
{
    return static_cast<TileCover>(~static_cast<std::underlying_type_t<TileCover>>(lhs));
}

inline constexpr TileCover &operator|=(TileCover &lhs, const TileCover rhs)
{
    lhs = static_cast<TileCover>(lhs | rhs);
    return lhs;
}

inline constexpr TileCover &operator&=(TileCover &lhs, const TileCover rhs)
{
    lhs = static_cast<TileCover>(lhs & rhs);
    return lhs;
}

inline constexpr TileCover &operator&=(TileCover &lhs, int x)
{
    lhs = static_cast<TileCover>(lhs & x);
    return lhs;
}

inline constexpr TileCover operator|(const TileCover &lhs, const TileCover &rhs)
{
    return static_cast<TileCover>(lhs | static_cast<std::underlying_type_t<TileCover>>(rhs));
}

inline constexpr TileCover operator*(const TileCover &lhs, const int value)
{
    return value ? lhs : TILE_COVER_NONE;
}

inline constexpr TileCover operator&(const TileCover &lhs, const TileCover &rhs)
{
    return static_cast<TileCover>(static_cast<std::underlying_type_t<TileCover>>(lhs) & static_cast<std::underlying_type_t<TileCover>>(rhs));
}

class TileCovers
{
  public:
    static bool contains(TileCover source, TileCover test) noexcept;
    static void clearCoverFlags(TileCover &cover, TileCover flags);
    static void eraseSide(TileCover &cover, TileCover side, TileCover preferredDiagonal = TILE_COVER_NONE);
    static TileCover mirrorNorth(TileCover tileCover, bool corner = true);
    static TileCover mirrorEast(TileCover cover);
    static TileCover mirrorEast(TileCover source, TileCover cover);
    static TileCover mirrorSouth(TileCover tileCover);
    static TileCover mirrorWest(TileCover cover);
    static TileCover mirrorWest(TileCover source, TileCover cover);
    static TileCover unifyTileCover(TileCover cover, TileQuadrant quadrant, TileCover preferredDiagonal = TILE_COVER_NONE);
    static TileCover mergeTileCover(TileCover a, TileCover b);
    static bool exactlyOneSet(TileCover cover)
    {
        auto bits = static_cast<std::underlying_type_t<TileCover>>(cover);
        return bits && !(bits & (bits - 1));
    }

    static TileCover fromBorderType(BorderType borderType);

    static bool hasFullSide(TileCover cover, TileCover side);
    static TileCover getFull(TileCover side);
    static TileCover addNonMasked(TileCover current, TileCover committed, TileCover mask);

    static TileCover mirrorXY(TileCover cover);

    static std::string show(TileCover cover);

    static constexpr TileCover None = TILE_COVER_NONE;
    static constexpr TileCover Full = TILE_COVER_FULL;
    static constexpr TileCover North = TILE_COVER_NORTH;
    static constexpr TileCover East = TILE_COVER_EAST;
    static constexpr TileCover South = TILE_COVER_SOUTH;
    static constexpr TileCover West = TILE_COVER_WEST;
    static constexpr TileCover NorthWest = TILE_COVER_NORTH_WEST;
    static constexpr TileCover NorthEast = TILE_COVER_NORTH_EAST;
    static constexpr TileCover SouthWest = TILE_COVER_SOUTH_WEST;
    static constexpr TileCover SouthEast = TILE_COVER_SOUTH_EAST;
    static constexpr TileCover NorthWestCorner = TILE_COVER_NORTH_WEST_CORNER;
    static constexpr TileCover NorthEastCorner = TILE_COVER_NORTH_EAST_CORNER;
    static constexpr TileCover SouthWestCorner = TILE_COVER_SOUTH_WEST_CORNER;
    static constexpr TileCover SouthEastCorner = TILE_COVER_SOUTH_EAST_CORNER;

    static constexpr TileCover Corners = TILE_COVER_NORTH_EAST_CORNER | TILE_COVER_NORTH_WEST_CORNER | TILE_COVER_SOUTH_EAST_CORNER | TILE_COVER_SOUTH_WEST_CORNER;
    static constexpr TileCover FullNorth = TILE_COVER_FULL | TILE_COVER_NORTH | TILE_COVER_NORTH_EAST | TILE_COVER_NORTH_WEST;
    static constexpr TileCover FullEast = TILE_COVER_FULL | TILE_COVER_EAST | TILE_COVER_NORTH_EAST | TILE_COVER_SOUTH_EAST;
    static constexpr TileCover FullSouth = TILE_COVER_FULL | TILE_COVER_SOUTH | TILE_COVER_SOUTH_EAST | TILE_COVER_SOUTH_WEST;
    static constexpr TileCover FullWest = TILE_COVER_FULL | TILE_COVER_WEST | TILE_COVER_SOUTH_WEST | TILE_COVER_NORTH_WEST;
    static constexpr TileCover Diagonals = TILE_COVER_NORTH_WEST | TILE_COVER_NORTH_EAST | TILE_COVER_SOUTH_EAST | TILE_COVER_SOUTH_WEST;

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

namespace TileCoverShortHands
{
    constexpr TileCover None = TileCovers::None;
    constexpr TileCover Full = TileCovers::Full;
    constexpr TileCover North = TileCovers::North;
    constexpr TileCover East = TileCovers::East;
    constexpr TileCover South = TileCovers::South;
    constexpr TileCover West = TileCovers::West;
    constexpr TileCover NorthWest = TileCovers::NorthWest;
    constexpr TileCover NorthEast = TileCovers::NorthEast;
    constexpr TileCover SouthWest = TileCovers::SouthWest;
    constexpr TileCover SouthEast = TileCovers::SouthEast;
    constexpr TileCover NorthWestCorner = TileCovers::NorthWestCorner;
    constexpr TileCover NorthEastCorner = TileCovers::NorthEastCorner;
    constexpr TileCover SouthWestCorner = TileCovers::SouthWestCorner;
    constexpr TileCover SouthEastCorner = TileCovers::SouthEastCorner;

    constexpr TileCover Corners = TileCovers::Corners;
    constexpr TileCover FullNorth = TileCovers::FullNorth;
    constexpr TileCover FullEast = TileCovers::FullEast;
    constexpr TileCover FullSouth = TileCovers::FullSouth;
    constexpr TileCover FullWest = TileCovers::FullWest;
    constexpr TileCover Diagonals = TileCovers::Diagonals;
}; // namespace TileCoverShortHands