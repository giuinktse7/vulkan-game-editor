#pragma once

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
    static void clearCoverFlags(TileCover &cover, TileCover flags);
    static void eraseSide(TileCover &cover, TileCover side, TileCover preferredDiagonal = TILE_COVER_NONE);
    static TileCover mirrorEast(TileCover source, TileCover cover);
    static TileCover mirrorWest(TileCover source, TileCover cover);
    static TileCover mirrorNorth(TileCover tileCover);
    static TileCover mirrorSouth(TileCover tileCover);
    static TileCover unifyTileCover(TileCover cover, TileQuadrant quadrant, TileCover preferredDiagonal = TILE_COVER_NONE);
    static TileCover mergeTileCover(TileCover a, TileCover b);
    static bool exactlyOneSet(TileCover cover)
    {
        auto bits = static_cast<std::underlying_type_t<TileCover>>(cover);
        return bits && !(bits & (bits - 1));
    }

    static bool hasFullSide(TileCover cover, TileCover side);
    static TileCover getFull(TileCover side);
    static TileCover addNonMasked(TileCover current, TileCover committed, TileCover mask);

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
};
