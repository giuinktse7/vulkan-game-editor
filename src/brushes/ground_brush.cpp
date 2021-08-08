
#include "ground_brush.h"

#include <algorithm>
#include <numeric>

#include "../config.h"
#include "../debug.h"
#include "../items.h"
#include "../map_view.h"
#include "../position.h"
#include "../random.h"
#include "../tile.h"
#include "../tile_cover.h"
#include "border_brush.h"

std::variant<std::monostate, uint32_t, const GroundBrush *> GroundBrush::replacementFilter = std::monostate{};

GroundBrush::GroundBrush(std::string id, const std::string &name, std::vector<WeightedItemId> &&weightedIds)
    : Brush(name), _weightedIds(std::move(weightedIds)), id(id), _iconServerId(_weightedIds.at(0).id)
{
    initialize();
}

GroundBrush::GroundBrush(std::string id, const std::string &name, std::vector<WeightedItemId> &&weightedIds, uint32_t iconServerId)
    : Brush(name), _weightedIds(std::move(weightedIds)), id(id), _iconServerId(iconServerId)
{
    initialize();
}

GroundBrush::GroundBrush(const std::string &id, std::vector<WeightedItemId> &&weightedIds, uint32_t zOrder)
    : Brush(id), _weightedIds(std::move(weightedIds)), id(id), _iconServerId(_weightedIds.at(0).id), _zOrder(zOrder)
{
    initialize();
}

void GroundBrush::setIconServerId(uint32_t serverId)
{
    _iconServerId = serverId;
}

void GroundBrush::initialize()
{
    // Sort by weights descending to optimize iteration in sampleServerId() for the most common cases.
    std::sort(_weightedIds.begin(), _weightedIds.end(), [](const WeightedItemId &a, const WeightedItemId &b) { return a.weight > b.weight; });

    for (auto &entry : _weightedIds)
    {
        totalWeight += entry.weight;
        entry.weight = totalWeight;
        _serverIds.emplace(entry.id);
    }
}

void GroundBrush::erase(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);
    mapView.removeItemsWithBorderize(tile, [this](const Item &item) { return this->erasesItem(item.serverId()); });
}

void GroundBrush::apply(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);

    if (mayPlaceOnTile(tile))
    {
        mapView.setGround(tile, Item(nextServerId()), true);

        if (Settings::AUTO_BORDER)
        {
            GroundNeighborMap neighbors(this, position, *mapView.map());
            neighbors.addCenterCorners();

            // Must manually set center because the new ground has not been committed to the map yet.
            TileBorderBlock center{};
            center.ground = this;
            neighbors.set(0, 0, center);

            int zOrder = this->zOrder();

            fixBorders(mapView, position, neighbors);
        }
    }
}

void GroundBrush::apply(MapView &mapView, const Position &position, const BorderBrush *brush, BorderType borderType)
{
    mapView.addItem(position, brush->getServerId(borderType));
}

void GroundBrush::fixBorders(MapView &mapView, const Position &position, GroundNeighborMap &neighbors)
{
    // Do (0, 0) first
    fixBordersAtOffset(mapView, position, neighbors, 0, 0);

    // Top
    fixBordersAtOffset(mapView, position, neighbors, -1, -1);
    fixBordersAtOffset(mapView, position, neighbors, 0, -1);
    fixBordersAtOffset(mapView, position, neighbors, 1, -1);

    // Middle
    fixBordersAtOffset(mapView, position, neighbors, -1, 0);
    fixBordersAtOffset(mapView, position, neighbors, 1, 0);

    // Bottom
    fixBordersAtOffset(mapView, position, neighbors, -1, 1);
    fixBordersAtOffset(mapView, position, neighbors, 0, 1);
    fixBordersAtOffset(mapView, position, neighbors, 1, 1);
}

void GroundBrush::fixBordersAtOffset(MapView &mapView, const Position &position, GroundNeighborMap &neighbors, int x, int y)
{
    using namespace TileCoverShortHands;

    auto pos = position + Position(x, y, 0);

    auto currentCover = neighbors.at(x, y);

    TileBorderBlock cover;
    cover.ground = currentCover.ground;

    GroundNeighborMap::mirrorNorth(cover, neighbors.at(x, y + 1));
    GroundNeighborMap::mirrorEast(cover, neighbors.at(x - 1, y));
    GroundNeighborMap::mirrorSouth(cover, neighbors.at(x, y - 1));
    GroundNeighborMap::mirrorWest(cover, neighbors.at(x + 1, y));

    GroundNeighborMap::mirrorNorthWest(cover, neighbors.at(x - 1, y - 1));
    GroundNeighborMap::mirrorNorthEast(cover, neighbors.at(x + 1, y - 1));
    GroundNeighborMap::mirrorSouthEast(cover, neighbors.at(x + 1, y + 1));
    GroundNeighborMap::mirrorSouthWest(cover, neighbors.at(x - 1, y + 1));

    // Do not use a mirrored diagonal if we already have a diagonal.
    for (auto &block : cover.borders)
    {
        auto current = currentCover.border(block.brush);
        if (current && current->cover & Diagonals)
        {
            block.cover &= ~(Diagonals);
        }
    }

    cover.merge(currentCover);

    auto quadrant = mapView.getMouseDownTileQuadrant();

    // TileQuadrant quadrant = getNeighborQuadrant(x, y);
    for (auto &block : cover.borders)
    {
        // Compute preferred diagonal
        TileCover preferredDiagonal = block.cover & Diagonals;
        if (x == 0 && y == 0)
        {
            if (quadrant)
            {
                switch (*quadrant)
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
        }
        block.cover = TileCovers::unifyTileCover(block.cover, TileQuadrant::TopLeft, preferredDiagonal);
    }

    // fixBorderEdgeCases(x, y, cover, neighbors);

    neighbors.set(x, y, cover);

    Tile &tile = mapView.getOrCreateTile(pos);
    mapView.removeItems(tile, [](const Item &item) {
        return item.isBorder();
    });

    cover.sort();

    for (const auto &block : cover.borders)
    {
        auto cover = block.cover;
        auto brush = block.brush;

        if (cover & Full)
        {
            apply(mapView, pos, brush, BorderType::North);
            apply(mapView, pos, brush, BorderType::East);
            apply(mapView, pos, brush, BorderType::South);
            apply(mapView, pos, brush, BorderType::West);
            continue;
        }

        // Sides
        if (cover & North)
        {
            apply(mapView, pos, brush, BorderType::North);
        }
        if (cover & East)
        {
            apply(mapView, pos, brush, BorderType::East);
        }
        if (cover & South)
        {
            apply(mapView, pos, brush, BorderType::South);
        }
        if (cover & West)
        {
            apply(mapView, pos, brush, BorderType::West);
        }

        // Diagonals
        if (cover & Diagonals)
        {
            if (cover & NorthWest)
            {
                apply(mapView, pos, brush, BorderType::NorthWestDiagonal);
            }
            else if (cover & NorthEast)
            {
                apply(mapView, pos, brush, BorderType::NorthEastDiagonal);
            }
            else if (cover & SouthEast)
            {
                apply(mapView, pos, brush, BorderType::SouthEastDiagonal);
            }
            else if (cover & SouthWest)
            {
                apply(mapView, pos, brush, BorderType::SouthWestDiagonal);
            }
        }

        // Corners
        if (cover & Corners)
        {
            if (cover & NorthEastCorner)
            {
                apply(mapView, pos, brush, BorderType::NorthEastCorner);
            }
            if (cover & NorthWestCorner)
            {
                apply(mapView, pos, brush, BorderType::NorthWestCorner);
            }
            if (cover & SouthEastCorner)
            {
                apply(mapView, pos, brush, BorderType::SouthEastCorner);
            }
            if (cover & SouthWestCorner)
            {
                apply(mapView, pos, brush, BorderType::SouthWestCorner);
            }
        }
    }
}

bool GroundBrush::mayPlaceOnTile(Tile &tile)
{
    if (replacementFilter.index() == 0)
    {
        return true;
    }

    bool result = false;
    std::visit(
        util::overloaded{
            [&tile, &result](uint32_t serverId) {
                Item *ground = tile.ground();
                if (ground && ground->serverId() == serverId)
                {
                    result = true;
                }
            },
            [&tile, &result](const GroundBrush *groundBrush) {
                if (groundBrush)
                {
                    if (tile.hasGround() && groundBrush == tile.ground()->itemType->brush)
                    {
                        result = true;
                    }
                }
                else
                {
                    // Only tiles without ground
                    if (!tile.hasGround())
                    {
                        result = true;
                    }
                }
            },
            [](const auto &arg) {}},
        replacementFilter);

    return result;
}

uint32_t GroundBrush::iconServerId() const
{
    return _iconServerId;
}

BrushType GroundBrush::type() const
{
    return BrushType::Ground;
}

bool GroundBrush::erasesItem(uint32_t serverId) const
{
    if (_serverIds.find(serverId) != _serverIds.end())
    {
        return true;
    }

    if (Settings::AUTO_BORDER)
    {

        for (const auto &border : borders)
        {
            if (border.brush->hasBorder(serverId))
            {
                return true;
            }
        }
    }

    return false;
}

uint32_t GroundBrush::nextServerId() const
{
    return sampleServerId();
}

uint32_t GroundBrush::sampleServerId() const
{
    uint32_t weight = Random::global().nextInt<uint32_t>(static_cast<uint32_t>(0), totalWeight);

    for (const auto &entry : _weightedIds)
    {
        if (weight < entry.weight)
        {
            return entry.id;
        }
    }

    // If we get here, something is off with `weight` for some weighted ID or with `totalWeight`.
    VME_LOG_ERROR(
        "[GroundBrush::nextServerId] Brush " << _name
                                             << ": Could not find matching weight for randomly generated weight "
                                             << weight << " (totalWeight: " << totalWeight << ".");

    // No match in for-loop. Use the first ID.
    return _weightedIds.at(0).id;
}

std::string GroundBrush::brushId() const noexcept
{
    return id;
}

void GroundBrush::setName(std::string name)
{
    _name = name;
}

std::vector<ThingDrawInfo> GroundBrush::getPreviewTextureInfo(int variation) const
{
    return std::vector<ThingDrawInfo>{DrawItemType(_iconServerId, PositionConstants::Zero)};
}

const std::string GroundBrush::getDisplayId() const
{
    return id;
}

const std::unordered_set<uint32_t> &GroundBrush::serverIds() const
{
    return _serverIds;
}

void GroundBrush::restrictReplacement(const Tile *tile)
{
    if (!tile || !tile->ground())
    {
        replacementFilter = static_cast<const GroundBrush *>(nullptr);
        return;
    }

    Item *ground = tile->ground();
    auto tileGroundBrush = ground->itemType->brush;
    if (tileGroundBrush && tileGroundBrush->type() == BrushType::Ground)
    {
        replacementFilter = static_cast<GroundBrush *>(tileGroundBrush);
    }
    else
    {
        replacementFilter = ground->serverId();
    }
}

void GroundBrush::disableReplacement()
{
    replacementFilter = static_cast<const GroundBrush *>(nullptr);
}

void GroundBrush::resetReplacementFilter()
{
    replacementFilter = std::monostate{};
}

bool GroundBrush::hasReplacementFilter() noexcept
{
    return replacementFilter.index() != 0;
}

void GroundBrush::addBorder(GroundBorder border)
{
    borders.emplace_back(border);
}

uint32_t GroundBrush::zOrder() const noexcept
{
    return _zOrder;
}

BorderBrush *GroundBrush::getBorderTowards(Tile *tile) const
{
    return tile ? getBorderTowards(tile->groundBrush()) : nullptr;
}

BorderBrush *GroundBrush::getBorderTowards(GroundBrush *groundBrush) const
{
    // TODO Place the default border last, otherwise it will always be picked up even if there's a more specific one.
    auto found = std::find_if(borders.begin(), borders.end(), [groundBrush](const GroundBorder &border) {
        return !border.to || *border.to == groundBrush;
    });

    return (found != borders.end()) ? found->brush : nullptr;
}

BorderBrush *GroundBrush::getDefaultOuterBorder() const
{
    // TODO Place the default border last
    auto found = std::find_if(borders.begin(), borders.end(), [](const GroundBorder &border) {
        return !border.to;
    });

    return (found != borders.end()) ? found->brush : nullptr;
}

GroundNeighborMap::GroundNeighborMap(GroundBrush *centerGround, const Position &position, const Map &map)
    : centerGround(centerGround)
{
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            TileCover mask = getExcludeMask(dx, dy);
            auto tileCovers = getTileCoverAt(map, position + Position(dx, dy, 0), mask);
            if (tileCovers)
            {
                for (auto &cover : tileCovers->borders)
                {
                    cover.cover = TileCovers::unifyTileCover(cover.cover, TileQuadrant::TopLeft);
                }
                set(dx, dy, *tileCovers);
            }
        }
    }
}

std::optional<TileBorderBlock> GroundNeighborMap::getTileCoverAt(const Map &map, const Position position, TileCover mask) const
{
    Tile *tile = map.getTile(position);

    if (!tile)
    {
        return std::nullopt;
    }

    auto borderBlock = tile->getFullBorderTileCover(mask);
    return borderBlock;
}

TileCover GroundNeighborMap::getExcludeMask(int dx, int dy)
{
    return indexToBorderExclusionMask[index(dx, dy)];
}

bool GroundNeighborMap::isExpanded(int x, int y) const
{
    auto found = std::find_if(expandedCovers.begin(), expandedCovers.end(), [x, y](const ExpandedTileBlock &block) { return block.x == x && block.y == y; });
    return found != expandedCovers.end();
}

bool GroundNeighborMap::hasExpandedCover() const noexcept
{
    return !expandedCovers.empty();
}

void GroundNeighborMap::addExpandedCover(int x, int y)
{
    expandedCovers.emplace_back(ExpandedTileBlock{x, y});
}

TileBorderBlock &GroundNeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

TileBorderBlock &GroundNeighborMap::center()
{
    return data[12];
}

TileBorderBlock GroundNeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

void GroundNeighborMap::set(int x, int y, TileBorderBlock tileCover)
{
    data[index(x, y)] = tileCover;
}

int GroundNeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}

void GroundNeighborMap::addGroundBorder(value_type &self, const value_type &other, TileCover border)
{
    if (other.ground)
    {
        if (self.ground)
        {
            if (self.zOrder() < other.zOrder())
            {
                auto brush = other.ground->getBorderTowards(self.ground);
                if (brush)
                {
                    self.add(border, brush);
                }
            }
        }
        else
        {
            auto brush = other.ground->getDefaultOuterBorder();
            if (brush)
            {
                self.add(border, brush);
            }
        }
    }
}

void GroundNeighborMap::mirrorNorth(TileBorderBlock &source, const TileBorderBlock &borders)
{
    uint32_t sourceZ = source.zOrder();
    for (const auto &border : borders.borders)
    {
        uint32_t z = border.brush->preferredZOrder();
        if (z > sourceZ)
        {
            TileCover cover = TileCovers::mirrorNorth(border.cover);
            source.add(cover, border.brush);
        }
    }

    addGroundBorder(source, borders, TILE_COVER_SOUTH);
}

void GroundNeighborMap::mirrorEast(TileBorderBlock &source, const TileBorderBlock &borders)
{
    uint32_t sourceZ = source.zOrder();
    for (const auto &border : borders.borders)
    {
        uint32_t z = border.brush->preferredZOrder();
        if (z > sourceZ)
        {
            TileCover cover = TileCovers::mirrorEast(border.cover);
            source.add(cover, border.brush);
        }
    }

    addGroundBorder(source, borders, TILE_COVER_WEST);
}

void GroundNeighborMap::mirrorSouth(value_type &source, const value_type &borders)
{
    uint32_t sourceZ = source.zOrder();
    for (const auto &border : borders.borders)
    {
        uint32_t z = border.brush->preferredZOrder();
        if (z > sourceZ)
        {
            TileCover cover = TileCovers::mirrorSouth(border.cover);
            source.add(cover, border.brush);
        }
    }

    addGroundBorder(source, borders, TILE_COVER_NORTH);
}

void GroundNeighborMap::mirrorWest(value_type &source, const value_type &borders)
{
    uint32_t sourceZ = source.zOrder();
    for (const auto &border : borders.borders)
    {
        uint32_t z = border.brush->preferredZOrder();
        if (z > sourceZ)
        {
            TileCover cover = TileCovers::mirrorWest(border.cover);
            source.add(cover, border.brush);
        }
    }

    addGroundBorder(source, borders, TILE_COVER_EAST);
}

void GroundNeighborMap::mirrorNorthWest(value_type &source, const value_type &borders)
{
    // auto otherGroundBorderBrush = borders.ground->getBorderTowards(source.ground);
    // uint32_t sourceZ = source.zOrder();
    // for (const auto &border : borders.borders)
    // {
    //     uint32_t z = border.brush->preferredZOrder();
    //     if (z > sourceZ)
    //     {
    //         if ((border.cover & TILE_COVER_FULL) || otherGroundBorderBrush == border.brush)
    //             source.add(TILE_COVER_NORTH_WEST, border.brush);
    //     }
    // }

    addGroundBorder(source, borders, TILE_COVER_NORTH_WEST_CORNER);
}

void GroundNeighborMap::mirrorNorthEast(value_type &source, const value_type &borders)
{
    addGroundBorder(source, borders, TILE_COVER_NORTH_EAST_CORNER);
}

void GroundNeighborMap::mirrorSouthEast(value_type &source, const value_type &borders)
{
    addGroundBorder(source, borders, TILE_COVER_SOUTH_EAST_CORNER);
}

void GroundNeighborMap::mirrorSouthWest(value_type &source, const value_type &borders)
{
    addGroundBorder(source, borders, TILE_COVER_SOUTH_WEST_CORNER);
}

void GroundNeighborMap::addCenterCorners()
{
    uint32_t zOrder = centerGround->zOrder();
    auto fix = [this, zOrder](int dx, int dy, TileCover corner) {
        auto &block = at(dx, dy);
        auto brush = centerGround->getBorderTowards(block.ground);

        if (brush && (!block.ground || block.ground->zOrder() < zOrder))
        {
            block.add(corner, brush);
        }
    };

    fix(-1, -1, TILE_COVER_SOUTH_EAST_CORNER);
    fix(1, -1, TILE_COVER_SOUTH_WEST_CORNER);
    fix(-1, 1, TILE_COVER_NORTH_EAST_CORNER);
    fix(1, 1, TILE_COVER_NORTH_WEST_CORNER);
}

const BorderZOrderBlock *TileBorderBlock::border(BorderBrush *brush) const
{
    for (const auto &border : borders)
    {
        if (border.brush == brush)
            return &border;
    }

    return nullptr;
}

void TileBorderBlock::add(const BorderZOrderBlock &block)
{
    add(block.cover, block.brush);
}

void TileBorderBlock::add(TileCover cover, BorderBrush *brush)
{
    auto found = std::find_if(borders.begin(), borders.end(), [brush](const BorderZOrderBlock &block) {
        return block.brush == brush;
    });

    if (found != borders.end())
    {
        found->cover |= cover;
    }
    else
    {
        borders.emplace_back(BorderZOrderBlock(cover, brush));
    }
}

void TileBorderBlock::merge(const TileBorderBlock &other)
{
    for (const auto &border : other.borders)
    {
        add(border.cover, border.brush);
    }
}

void TileBorderBlock::sort()
{
    std::sort(borders.begin(), borders.end(), [](BorderZOrderBlock &lhs, BorderZOrderBlock &rhs) {
        return lhs.brush->preferredZOrder() < rhs.brush->preferredZOrder();
    });
}

uint32_t TileBorderBlock::zOrder() const noexcept
{
    return ground ? ground->zOrder() : 0;
}

void GroundBrush::borderize(MapView &mapView, const Position &position)
{
    using namespace TileCoverShortHands;

    GroundNeighborMap neighbors(nullptr, position, *mapView.map());

    int dx = -1;
    int dy = -1;

    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            auto &center = neighbors.at(dx, dy);
            for (auto &block : center.borders)
            {
                TileCover remove = None;

#define remove_invalid_border(x, y, requiredCover, removeCover)           \
    do                                                                    \
    {                                                                     \
        auto neighbor = neighbors.at(dx + x, dy + y).border(block.brush); \
        if (!(neighbor && (neighbor->cover & (requiredCover))))           \
        {                                                                 \
            remove |= (removeCover);                                      \
        }                                                                 \
    } while (false)

                remove_invalid_border(-1, -1, NorthEast | SouthWest | East | South | SouthEastCorner, NorthWestCorner);
                remove_invalid_border(0, -1, FullSouth, North);
                remove_invalid_border(1, -1, NorthWest | SouthEast | West | South | SouthWestCorner, NorthEastCorner);
                remove_invalid_border(1, 0, FullWest, East);
                remove_invalid_border(1, 1, NorthWest | SouthWest | West | North | NorthWestCorner, SouthEastCorner);
                remove_invalid_border(0, 1, FullNorth, South);
                remove_invalid_border(-1, 1, NorthWest | SouthEast | West | North | NorthEastCorner, SouthWestCorner);
                remove_invalid_border(-1, 0, FullEast, West);

                if (remove != None)
                {
                    block.cover &= ~remove;
                }
            }
        }
    }

    fixBorders(mapView, position, neighbors);
}
