#include "border_brush.h"

#include <algorithm>

#include "../items.h"
#include "../map_view.h"
#include "../settings.h"
#include "../util.h"
#include "ground_brush.h"
#include "raw_brush.h"

TileBorderInfo BorderBrush::tileInfo = {};
BorderBrushVariation *BorderBrush::brushVariation = &DetailedBorderBrush::instance;

BorderExpandDirection BorderBrush::currDir;
std::optional<BorderExpandDirection> BorderBrush::prevDir;

using TileQuadrant::TopLeft, TileQuadrant::TopRight, TileQuadrant::BottomRight, TileQuadrant::BottomLeft;

inline std::ostream &operator<<(std::ostream &os, const TileQuadrant &quadrant)
{
    switch (quadrant)
    {
        case TopLeft:
            os << "TopLeft";
            break;
        case TopRight:
            os << "TopRight";
            break;
        case BottomRight:
            os << "BottomRight";
            break;
        case BottomLeft:
            os << "BottomLeft";
            break;
    }

    return os;
}

TileQuadrant mirrorQuadrant(TileQuadrant quadrant, bool horizontal = true)
{
    switch (quadrant)
    {
        case TileQuadrant::TopLeft:
            return horizontal ? TopRight : BottomLeft;
        case TileQuadrant::TopRight:
            return horizontal ? TopLeft : BottomRight;
        case TileQuadrant::BottomRight:
            return horizontal ? BottomLeft : TopRight;
        case TileQuadrant::BottomLeft:
            return horizontal ? BottomRight : TopLeft;
        default:
            ABORT_PROGRAM("Bad quadrant.");
    }
}

std::pair<int, int> quadrantDiff(TileQuadrant lhs, TileQuadrant rhs)
{

    int x0 = to_underlying(lhs & (TopLeft | BottomLeft)) ? 0 : 1;
    int x1 = to_underlying(rhs & (TopLeft | BottomLeft)) ? 0 : 1;

    int y0 = to_underlying(lhs & (TopLeft | TopRight)) ? 0 : 1;
    int y1 = to_underlying(rhs & (TopLeft | TopRight)) ? 0 : 1;

    return {x0 - x1, y0 - y1};
}

bool clockwise(TileQuadrant a, TileQuadrant b)
{

    return (a == TopLeft && b == TopRight) || (a == TopRight && b == BottomRight) || (a == BottomRight && b == BottomLeft) || (a == BottomLeft && b == TopLeft);
}

TileQuadrant oppositeQuadrant(TileQuadrant quadrant)
{

    if (quadrant == TopLeft)
        return BottomRight;
    else if (quadrant == TopRight)
        return BottomLeft;
    else if (quadrant == BottomLeft)
        return TopRight;
    else
        return TopLeft;
}

bool isOpposite(TileQuadrant a, TileQuadrant b)
{

    TileQuadrant both = (a | b);
    return (both == (BottomLeft | TopRight)) || (both == (TopLeft | BottomRight));
}

BorderExpandDirection getTravelledSide(TileQuadrant from, TileQuadrant to)
{
    TileQuadrant quadrants = from | to;
    if (quadrants == (TopLeft | TopRight))
    {
        return BorderExpandDirection::North;
    }
    else if (quadrants == (TopRight | BottomRight))
    {
        return BorderExpandDirection::East;
    }
    else if (quadrants == (BottomRight | BottomLeft))
    {
        return BorderExpandDirection::South;
    }
    else if (quadrants == (BottomLeft | TopLeft))
    {
        return BorderExpandDirection::West;
    }
    else
    {
        return BorderExpandDirection::None;
    }
}

bool isTop(TileQuadrant quadrant);
bool isLeft(TileQuadrant quadrant);
bool isRight(TileQuadrant quadrant);

BorderBrush::BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds, GroundBrush *centerBrush)
    : Brush(name), id(id), borderData(borderIds, centerBrush)
{
    initialize();
}

BorderBrush::BorderBrush(std::string id, const std::string &name, std::array<uint32_t, 12> borderIds)
    : Brush(name), id(id), borderData(borderIds)
{
    initialize();
}

void BorderBrush::initialize()
{
    for (uint32_t id : borderData.getBorderIds())
    {
        sortedServerIds.emplace_back(id);
    }

    const auto &extraIds = borderData.getExtraBorderIds();
    if (extraIds)
    {
        for (const auto &[key, val] : *extraIds)
        {
            sortedServerIds.emplace_back(key);
        }
    }

    std::sort(sortedServerIds.begin(), sortedServerIds.end());

    auto centerBrush = borderData.centerBrush();
    if (centerBrush && centerBrush->type() == BrushType::Ground)
    {
        zOrder = static_cast<GroundBrush *>(centerBrush)->zOrder();
    }
}

void BorderBrush::apply(MapView &mapView, const Position &position)
{
    using Dir = BorderExpandDirection;

    auto mouseQuadrant = mapView.getMouseDownTileQuadrant();

    // Need a mouse quadrant to apply
    if (!mouseQuadrant)
    {
        return;
    }

    const Map &map = *mapView.map();
    BorderNeighborMap neighbors(position, this, map);

    std::string dirName = "None";

    Position prevPos = mapView.getLastBrushDragPosition().value_or(position);
    Position expandDirection = position - prevPos;

    if (expandDirection == PositionConstants::Zero)
    {
        TileQuadrant quadrant = *mouseQuadrant;
        brushVariation->expandCenter(neighbors, tileInfo, quadrant);

        prevDir.reset();
        currDir = Dir::None;

        tileInfo.quadrant = quadrant;
        tileInfo.borderStartQuadrant = quadrant;
    }
    else
    {
        prevDir = currDir;

        if (expandDirection.x == -1)
        {
            dirName = "West";
            brushVariation->expandWest(neighbors, tileInfo, *prevDir);
            currDir = Dir::West;
        }
        else if (expandDirection.x == 1)
        {
            dirName = "East";
            brushVariation->expandEast(neighbors, tileInfo, *prevDir);
            currDir = Dir::East;
        }
        else if (expandDirection.y == -1)
        {
            dirName = "North";
            brushVariation->expandNorth(neighbors, tileInfo, *prevDir);
            currDir = Dir::North;
        }
        else if (expandDirection.y == 1)
        {
            dirName = "South";
            brushVariation->expandSouth(neighbors, tileInfo, *prevDir);
            currDir = Dir::South;
        }
    }

    // VME_LOG_D("Apply " << dirName << " :: " << position << "\tborderStart: " << tileInfo.borderStartQuadrant);

    updateCenter(neighbors);

    fixBorders(mapView, position, neighbors);

    tileInfo.cover = neighbors.center();
    tileInfo.prevQuadrant = tileInfo.quadrant;

    // VME_LOG_D("Set to " << tileInfo.quadrant << " in apply");

    if (tileInfo.borderStartQuadrant != *mouseQuadrant)
    {
        quadrantChanged(mapView, position, neighbors, tileInfo.borderStartQuadrant, *mouseQuadrant);
    }
}

void BorderBrush::quadrantChangeInFirstTile(TileCover cover, TileCover &nextCover, TileQuadrant prevQuadrant, TileQuadrant currQuadrant)
{
    using namespace TileCoverShortHands;
    TileCover horizontalCover = isTop(tileInfo.borderStartQuadrant) ? North : South;
    TileCover verticalCover = isLeft(tileInfo.borderStartQuadrant) ? West : East;
    TileQuadrant opposite = oppositeQuadrant(tileInfo.borderStartQuadrant);
    TileQuadrant siblingX = mirrorQuadrant(tileInfo.borderStartQuadrant);

    if (currQuadrant == opposite)
    {
        if (prevQuadrant != tileInfo.borderStartQuadrant)
        {
            if (prevQuadrant == siblingX)
            {
                auto side = verticalCover == West ? East : West;
                if (nextCover & side)
                {
                    nextCover &= ~side;
                }
                else
                {
                    nextCover |= side;
                }
            }
            else
            {
                auto side = horizontalCover == North ? South : North;
                if (nextCover & side)
                {
                    nextCover &= ~side;
                }
                else
                {
                    nextCover |= side;
                }
            }
        }
    }
    else if (prevQuadrant == opposite)
    {
        if (currQuadrant == siblingX)
        {
            auto side = verticalCover == West ? East : West;
            if (TileCovers::hasFullSide(cover, horizontalCover))
            {
                nextCover &= ~TileCovers::getFull(side);
                nextCover |= horizontalCover;
            }
            else
            {
                nextCover |= side;
            }
        }
        else
        {
            auto side = horizontalCover == North ? South : North;
            if (TileCovers::hasFullSide(cover, verticalCover))
            {
                nextCover &= ~TileCovers::getFull(side);
                nextCover |= verticalCover;
            }
            else
            {
                nextCover |= side;
            }
        }
    }
    else if (prevQuadrant == tileInfo.borderStartQuadrant)
    {
        nextCover |= currQuadrant == siblingX ? horizontalCover : verticalCover;
    }

    nextCover |= tileInfo.cover;
}

void BorderBrush::quadrantChanged(MapView &mapView, const Position &position, TileQuadrant prevQuadrant, TileQuadrant currQuadrant)
{
    const Map &map = *mapView.map();
    BorderNeighborMap neighbors(position, this, map);

    quadrantChanged(mapView, position, neighbors, prevQuadrant, currQuadrant);
}

void BorderBrush::quadrantChanged(MapView &mapView, const Position &position, BorderNeighborMap &neighbors, TileQuadrant prevQuadrant, TileQuadrant currQuadrant)
{
    TileCover &center = neighbors.center();
    TileCover nextCenter = brushVariation->quadrantChanged(neighbors, tileInfo, currDir, prevQuadrant, currQuadrant);

    if (center != nextCenter)
    {
        center = TileCovers::unifyTileCover(nextCenter, currQuadrant);
        neighbors.addExpandedCover(0, 0);

        fixBordersAtOffset(mapView, position, neighbors, 0, 0);
    }

    tileInfo.prevQuadrant = prevQuadrant;
    tileInfo.quadrant = currQuadrant;
}

void BorderBrush::updateCenter(BorderNeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    using Dir = BorderExpandDirection;

    TileCover &self = neighbors.at(0, 0);
    TileCover originalSelf = self;

    TileQuadrant quadrant = tileInfo.quadrant;

    // Update diagonal
    if (self & TileCovers::Diagonals)
    {
        if (quadrant == TileQuadrant::TopLeft)
        {
            if ((self & SouthWest) && (self & North))
            {
                self &= ~SouthWest;
                self |= NorthWest | South;
            }
            else if ((self & NorthEast) && (self & West))
            {
                self &= ~(NorthEast | West);
                self |= NorthWest | East;
            }
        }
        else if (quadrant == TileQuadrant::TopRight)
        {
            if ((self & NorthWest) && (self & East))
            {
                self &= ~NorthWest;
                self |= NorthEast | West;
            }
            else if ((self & SouthEast) && (self & North))
            {
                self &= ~SouthEast;
                self |= NorthEast | South;
            }
        }
        else if (quadrant == TileQuadrant::BottomRight)
        {
            if ((self & NorthEast) && (self & South))
            {
                self &= ~NorthEast;
                self |= SouthEast | North;
            }
            else if ((self & SouthWest) && (self & East))
            {
                self &= ~SouthWest;
                self |= SouthEast | West;
            }
        }
        else if (quadrant == TileQuadrant::BottomLeft)
        {
            if ((self & NorthWest) && (self & South))
            {
                self &= ~NorthWest;
                self |= SouthWest | North;
            }
            else if ((self & SouthEast) && (self & West))
            {
                self &= ~SouthEast;
                self |= SouthWest | East;
            }
        }
    }

    if (!neighbors.isExpanded(0, 0) && self != originalSelf)
    {
        neighbors.addExpandedCover(0, 0);
    }
}

void BorderBrush::fixBorders(MapView &mapView, const Position &position, BorderNeighborMap &neighbors)
{
    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            if (neighbors.isExpanded(dx, dy))
            {
                fixBordersAtOffset(mapView, position, neighbors, dx, dy);
            }
        }
    }
}

void BorderBrush::fixBorderEdgeCases(int x, int y, TileCover &cover, const BorderNeighborMap &neighbors)
{
    // Two of the same diagonal in a row
    if (cover & TILE_COVER_SOUTH_WEST)
    {
        if (neighbors.at(x, y - 1) & TILE_COVER_SOUTH_WEST)
        {
            TileCovers::clearCoverFlags(cover, TILE_COVER_SOUTH_WEST | TILE_COVER_NORTH);
            cover |= TILE_COVER_NORTH_WEST;
            cover |= TILE_COVER_SOUTH;
        }
    }
}

void BorderBrush::fixBordersAtOffset(MapView &mapView, const Position &position, BorderNeighborMap &neighbors, int x, int y)
{
    using namespace TileCoverShortHands;

    auto pos = position + Position(x, y, 0);

    auto currentCover = neighbors.at(x, y);

    TileCover cover = None;
    if (currentCover == Full)
    {
        cover = Full;
    }
    else
    {
        cover |= TileCovers::mirrorNorth(neighbors.at(x, y + 1));
        cover |= TileCovers::mirrorEast(cover, neighbors.at(x - 1, y));
        cover |= TileCovers::mirrorSouth(neighbors.at(x, y - 1));
        cover |= TileCovers::mirrorWest(cover, neighbors.at(x + 1, y));

        // Do not use a mirrored diagonal if we already have a diagonal.
        if (currentCover & Diagonals)
        {
            cover &= ~(Diagonals);
        }

        cover = TileCovers::mergeTileCover(cover, currentCover);
    }

    // Compute preferred diagonal
    TileCover preferredDiagonal = currentCover & Diagonals;
    if (x == 0 && y == 0)
    {
        switch (tileInfo.quadrant)
        {
            case TileQuadrant::TopLeft:
                preferredDiagonal = NorthWest;
                break;
            case TileQuadrant::TopRight:
                preferredDiagonal = NorthEast;
                break;
            case TileQuadrant::BottomRight:
                preferredDiagonal = SouthEast;
                break;
            case TileQuadrant::BottomLeft:
                preferredDiagonal = SouthWest;
                break;
        }
    }
    TileQuadrant quadrant = getNeighborQuadrant(x, y);
    cover = TileCovers::unifyTileCover(cover, quadrant, preferredDiagonal);
    fixBorderEdgeCases(x, y, cover, neighbors);

    if (cover == currentCover && (!neighbors.isExpanded(x, y)))
    {
        // No change necessary
        return;
    }

    neighbors.set(x, y, cover);

    Tile &tile = mapView.getOrCreateTile(pos);
    mapView.removeItems(tile, [this](const Item &item) {
        return this->includes(item.serverId());
    });

    if (cover == TILE_COVER_NONE)
    {
        return;
    }

    if (cover & Full)
    {
        apply(mapView, pos, BorderType::Center);
    }
    else
    {
        // Sides
        if (cover & North)
        {
            apply(mapView, pos, BorderType::North);
        }
        if (cover & East)
        {
            apply(mapView, pos, BorderType::East);
        }
        if (cover & South)
        {
            apply(mapView, pos, BorderType::South);
        }
        if (cover & West)
        {
            apply(mapView, pos, BorderType::West);
        }

        // Diagonals
        if (cover & Diagonals)
        {
            if (cover & NorthWest)
            {
                apply(mapView, pos, BorderType::NorthWestDiagonal);
            }
            else if (cover & NorthEast)
            {
                apply(mapView, pos, BorderType::NorthEastDiagonal);
            }
            else if (cover & SouthEast)
            {
                apply(mapView, pos, BorderType::SouthEastDiagonal);
            }
            else if (cover & SouthWest)
            {
                apply(mapView, pos, BorderType::SouthWestDiagonal);
            }
        }

        // Corners
        if (cover & Corners)
        {
            if (cover & NorthEastCorner)
            {
                apply(mapView, pos, BorderType::NorthEastCorner);
            }
            if (cover & NorthWestCorner)
            {
                apply(mapView, pos, BorderType::NorthWestCorner);
            }
            if (cover & SouthEastCorner)
            {
                apply(mapView, pos, BorderType::SouthEastCorner);
            }
            if (cover & SouthWestCorner)
            {
                apply(mapView, pos, BorderType::SouthWestCorner);
            }
        }
    }
}

void BorderBrush::erase(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);

    if (Settings::AUTO_BORDER)
    {
        mapView.removeItemsWithBorderize(tile, [this](const Item &item) { return this->erasesItem(item.serverId()); });
    }
    else
    {
        switch (Settings::BORDER_BRUSH_VARIATION)
        {
            case BorderBrushVariationType::Detailed:
                // TODO Only erase borders that overlap the hovered quadrant
                mapView.removeItems(tile, [this](const Item &item) { return this->erasesItem(item.serverId()); });
                break;
            default:
                mapView.removeItems(tile, [this](const Item &item) { return this->erasesItem(item.serverId()); });
        }
    }
}

void BorderBrush::apply(MapView &mapView, const Position &position, BorderType borderType)
{
    if (borderType == BorderType::Center)
    {
        Brush *centerBrush = borderData.centerBrush();
        if (centerBrush)
        {
            centerBrush->apply(mapView, position);
        }
    }
    else
    {
        auto borderItemId = borderData.getServerId(borderType);
        if (borderItemId)
        {
            mapView.addBorder(position, *borderItemId, this->preferredZOrder());
        }
    }
}

bool BorderBrush::presentAt(const Map &map, const Position position) const
{
    Tile *tile = map.getTile(position);
    if (!tile)
    {
        return false;
    }

    return tile->containsBorder(this);
}

uint32_t BorderBrush::borderItemAt(const Map &map, const Position position) const
{
    Tile *tile = map.getTile(position);
    if (!tile)
    {
        return 0;
    }

    return tile->getBorderServerId(this);
}

TileQuadrant BorderBrush::getNeighborQuadrant(int dx, int dy)
{
    if (dx == 0 && dy == 0)
        return tileInfo.quadrant;

    if (dx < 0)
    {
        return dy < 0 ? TileQuadrant::BottomRight : TileQuadrant::TopRight;
    }
    else
    {
        return dy < 0 ? TileQuadrant::BottomLeft : TileQuadrant::TopLeft;
    }
}

bool BorderBrush::includes(uint32_t serverId) const
{
    auto centerBrush = borderData.centerBrush();
    return hasBorder(serverId) || (centerBrush && centerBrush->erasesItem(serverId));
}

bool BorderBrush::hasBorder(uint32_t serverId) const
{
    return std::binary_search(sortedServerIds.begin(), sortedServerIds.end(), serverId);
}

bool BorderBrush::erasesItem(uint32_t serverId) const
{
    return includes(serverId);
}

BrushType BorderBrush::type() const
{
    return BrushType::Border;
}

const std::string BorderBrush::getDisplayId() const
{
    return id;
}

std::vector<ThingDrawInfo> BorderBrush::getPreviewTextureInfo(int variation) const
{
    // TODO improve preview
    return std::vector<ThingDrawInfo>{DrawItemType(_iconServerId, PositionConstants::Zero)};
}

uint32_t BorderBrush::iconServerId() const
{
    return _iconServerId;
}

void BorderBrush::setIconServerId(uint32_t serverId)
{
    _iconServerId = serverId;
}

std::string BorderBrush::brushId() const noexcept
{
    return id;
}

const std::vector<uint32_t> &BorderBrush::serverIds() const
{
    return sortedServerIds;
}

std::optional<uint32_t> BorderBrush::getServerId(BorderType borderType) const noexcept
{
    return borderData.getServerId(borderType);
}

GroundBrush *BorderBrush::centerBrush() const
{
    return borderData.centerBrush();
}

BorderType BorderBrush::getBorderType(uint32_t serverId) const
{
    return borderData.getBorderType(serverId);
}

TileCover BorderBrush::getTileCover(uint32_t serverId) const
{
    BorderType borderType = getBorderType(serverId);
    return TileCovers::fromBorderType(borderType);
}

bool isRight(TileQuadrant quadrant)
{
    return quadrant == TileQuadrant::TopRight || quadrant == TileQuadrant::BottomRight;
}
bool isLeft(TileQuadrant quadrant)
{
    return quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::BottomLeft;
}
bool isTop(TileQuadrant quadrant)
{
    return quadrant == TileQuadrant::TopLeft || quadrant == TileQuadrant::TopRight;
}

void BorderBrush::setBrushVariation(BorderBrushVariationType brushVariationType)
{
    Settings::BORDER_BRUSH_VARIATION = brushVariationType;

    switch (brushVariationType)
    {
        case BorderBrushVariationType::General:
            brushVariation = &GeneralBorderBrush::instance;
            break;
        case BorderBrushVariationType::Detailed:
            brushVariation = &DetailedBorderBrush::instance;
            break;
    }
}

void BorderBrush::setCenterGroundId(const std::string &id)
{
    borderData.setCenterGroundId(id);
}

uint32_t BorderBrush::preferredZOrder() const
{
    if (zOrder == std::numeric_limits<uint32_t>::max())
    {
        auto centerBrush = borderData.centerBrush();
        if (centerBrush && centerBrush->type() == BrushType::Ground)
        {
            zOrder = static_cast<GroundBrush *>(centerBrush)->zOrder();
        }
        else
        {
            zOrder = 0;
        }
    }

    return zOrder;
}

BorderStackBehavior BorderBrush::stackBehavior() const noexcept
{
    return _stackBehavior;
}

void BorderBrush::setStackBehavior(BorderStackBehavior behavior) noexcept
{
    _stackBehavior = behavior;
}

bool BorderRule::Condition::check(const TileBorderBlock &block) const
{
    for (const auto &cover : block.covers)
    {
        if (cover.brush->brushId() == borderId)
        {
            if (this->edges)
            {
                return TileCovers::contains(cover.cover, *this->edges);
            }
            else
            {
                return cover.cover != TILE_COVER_NONE;
            }
        }
    }

    return false;
}

void BorderRule::apply(MapView &mapView, BorderBrush *brush, const Position &position) const
{
    Tile &tile = mapView.getOrCreateTile(position);
    if (action == "setFull")
    {
        mapView.removeItems(tile, [brush](const Item &item) { return brush->erasesItem(item.serverId()); });
        if (brush->centerBrush())
        {
            brush->centerBrush()->applyWithoutBorderize(mapView, position);
        }
    }
}

std::optional<TileCover> WhenBorderRule::check(const TileBorderBlock &block) const
{
    for (const auto &cover : block.covers)
    {
        if (cover.brush->brushId() == borderId)
        {
            return cover.cover;
        }
    }

    return std::nullopt;
}

void ReplaceAction::apply(MapView &mapView, const Position &position, uint32_t oldServerId)
{
    Tile &tile = mapView.getOrCreateTile(position);
    mapView.replaceItemByServerId(tile, oldServerId, serverId);
}

void SetFullAction::apply(MapView &mapView, const Position &position, GroundBrush *groundBrush)
{
    groundBrush->applyWithoutBorderize(mapView, position);
}