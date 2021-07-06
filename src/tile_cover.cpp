#include "tile_cover.h"

void TileCovers::clearCoverFlags(TileCover &cover, TileCover flags)
{
    cover &= ~flags;
}

TileCover TileCovers::mergeTileCover(TileCover a, TileCover b)
{
    if (!(a & Diagonals))
    {
        return a | b;
    }
    else
    {
        a |= North * (b & (NorthWest | NorthEast));
        a |= East * (b & (NorthEast | SouthEast));
        a |= South * (b & (SouthEast | SouthWest));
        a |= West * (b & (SouthWest | NorthWest));
        return a | (b & (~Diagonals));
    }
}

TileCover TileCovers::unifyTileCover(TileCover cover, TileQuadrant quadrant, TileCover preferredDiagonal)
{
    DEBUG_ASSERT((preferredDiagonal == None) || exactlyOneSet(preferredDiagonal & Diagonals), "Preferred diagonal must be None or have exactly one diagonal set.");

    if (cover & Full)
        return Full;

    bool north = cover & FullNorth;
    bool east = cover & FullEast;
    bool south = cover & FullSouth;
    bool west = cover & FullWest;

    if (north && east && south && west)
    {
        return Full;
    }

    TileCover clear = None;

    if (north)
    {
        clear |= North * (cover & (NorthEast | NorthWest));
        clear |= NorthEastCorner | NorthWestCorner;
    }
    if (east)
    {
        clear |= East * (cover & (NorthEast | SouthEast));
        clear |= NorthEastCorner | SouthEastCorner;
    }
    if (south)
    {
        clear |= South * (cover & (SouthEast | SouthWest));
        clear |= SouthEastCorner | SouthWestCorner;
    }
    if (west)
    {
        clear |= West * (cover & (NorthWest | SouthWest));
        clear |= NorthWestCorner | SouthWestCorner;
    }
    clearCoverFlags(cover, clear);
    clear = None;

    TileCover diagonals = cover & Diagonals;
    if (diagonals)
    {
        // Exactly two diagonals
        if (!(TileCovers::exactlyOneSet(diagonals)))
        {
            // Double north
            if ((diagonals & NorthWest) && (diagonals & NorthEast))
            {
                bool left = (preferredDiagonal != NorthEast) && ((preferredDiagonal == NorthWest) || quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::BottomLeft);
                clear |= left ? NorthEast : NorthWest;
                cover |= left ? East : West;
            }
            // Double east
            else if ((diagonals & NorthEast) && (diagonals & SouthEast))
            {
                bool top = (preferredDiagonal != SouthEast) && ((preferredDiagonal == NorthEast) || quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::TopRight);
                clear |= top ? SouthEast : NorthEast;
                cover |= top ? South : North;
            }
            // Double south
            else if ((diagonals & SouthEast) && (diagonals & SouthWest))
            {
                bool left = (preferredDiagonal != SouthEast) && ((preferredDiagonal == SouthWest) || quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::BottomLeft);
                clear |= left ? SouthEast : SouthWest;
                cover |= left ? East : West;
            }
            // Double west
            else
            {
                bool top = (preferredDiagonal != SouthWest) && ((preferredDiagonal == NorthWest) || quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::TopRight);
                clear |= top ? SouthWest : NorthWest;
                cover |= top ? South : North;
            }

            clearCoverFlags(cover, clear);
            clear = None;
        }
    }
    else
    {
        if (cover & North)
        {
            if (cover & East)
            {
                cover |= NorthEast;
                clear |= North | East;
            }
            else if (cover & West)
            {
                cover |= NorthWest;
                clear |= North | West;
            }
        }
        else if (cover & South)
        {
            if (cover & East)
            {
                cover |= SouthEast;
                clear |= South | East;
            }
            else if (cover & West)
            {
                cover |= SouthWest;
                clear |= South | West;
            }
        }

        clearCoverFlags(cover, clear);
    }

    auto finalDiagonals = cover & Diagonals;
    DEBUG_ASSERT((finalDiagonals == None) || exactlyOneSet(finalDiagonals), "Preferred diagonal must be None or have exactly one diagonal set.");

    return cover;
}

TileCover TileCovers::mirrorEast(TileCover source, TileCover cover)
{
    if (cover & SouthEast)
    {
        return (source & South) ? SouthWest : West;
    }
    else if (cover & NorthEast)
    {
        return (source & North) ? NorthWest : West;
    }
    else
    {
        if (cover & FullEast)
        {
            return West;
        }

        TileCover result = None;
        if (cover & (North | NorthEastCorner | NorthWest))
        {
            result |= NorthWestCorner;
        }
        if (cover & (South | SouthEastCorner | SouthWest))
        {
            result |= SouthWestCorner;
        }

        return result;
    }
}

TileCover TileCovers::mirrorWest(TileCover source, TileCover cover)
{
    if (cover & SouthWest)
    {
        return (source & South) ? SouthEast : East;
    }
    else if (cover & NorthWest)
    {
        return (source & North) ? NorthEast : East;
    }
    else
    {
        if (cover & FullWest)
        {
            return East;
        }

        TileCover result = None;
        if (cover & (North | NorthWestCorner | NorthEast))
        {
            result |= NorthEastCorner;
        }
        if (cover & (South | SouthWestCorner | SouthEast))
        {
            result |= SouthEastCorner;
        }

        return result;
    }
}

TileCover TileCovers::mirrorNorth(TileCover tileCover)
{
    if (tileCover & FullNorth)
    {
        return South;
    }

    TileCover result = None;
    if (tileCover & (East | NorthEastCorner | SouthEast))
    {
        result |= SouthEastCorner;
    }
    if (tileCover & (West | NorthWestCorner | SouthWest))
    {
        result |= SouthWestCorner;
    }

    return result;
}

TileCover TileCovers::mirrorSouth(TileCover tileCover)
{
    if (tileCover & FullSouth)
    {
        return North;
    }

    TileCover result = None;
    if (tileCover & (East | SouthEastCorner | NorthEast))
    {
        result |= NorthEastCorner;
    }
    else if (tileCover & (West | SouthWestCorner | NorthWest))
    {
        result |= NorthWestCorner;
    }

    return result;
}