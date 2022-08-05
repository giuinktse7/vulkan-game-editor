
#include "ground_brush.h"

#include <algorithm>
#include <numeric>

#include "../debug.h"
#include "../items.h"
#include "../map_view.h"
#include "../position.h"
#include "../random.h"
#include "../settings.h"
#include "../tile.h"
#include "../tile_cover.h"
#include "border_brush.h"
#include "mountain_brush.h"

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

void GroundBrush::applyWithoutBorderize(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);

    if (mayPlaceOnTile(tile))
    {
        mapView.setGround(tile, Item(nextServerId()), true);
    }
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
            // neighbors.addCenterCorners();

            // Must manually set center because the new ground has not been committed to the map yet.
            TileBorderBlock center{};
            center.ground = this;
            neighbors.set(0, 0, center);

            int zOrder = this->zOrder();

            preBorderize(mapView, position, neighbors);
            fixBorders(mapView, position, neighbors);
            postBorderize(mapView, position, neighbors);

            MountainBrush::generalBorderize(mapView, position);
        }
    }
}

void GroundBrush::apply(MapView &mapView, const Position &position, const BorderBrush *brush, BorderType borderType)
{
    if (borderType == BorderType::Center && brush->centerBrush())
    {
        brush->centerBrush()->applyWithoutBorderize(mapView, position);
    }
    else
    {
        auto borderItemId = brush->getServerId(borderType);
        if (borderItemId)
        {
            mapView.addItem(position, *borderItemId);
        }
    }
}

void GroundBrush::fixBorders(MapView &mapView, const Position &position, GroundNeighborMap &neighbors)
{
    // Do (0, 0) first
    fixBordersAtOffset(mapView, position, neighbors, 0, 0);

    // left/right/up/down
    fixBordersAtOffset(mapView, position, neighbors, 0, -1);
    fixBordersAtOffset(mapView, position, neighbors, -1, 0);
    fixBordersAtOffset(mapView, position, neighbors, 1, 0);
    fixBordersAtOffset(mapView, position, neighbors, 0, 1);

    // diagonals
    fixBordersAtOffset(mapView, position, neighbors, -1, -1);
    fixBordersAtOffset(mapView, position, neighbors, 1, -1);
    fixBordersAtOffset(mapView, position, neighbors, -1, 1);
    fixBordersAtOffset(mapView, position, neighbors, 1, 1);
}

void GroundBrush::fixBordersAtOffset(MapView &mapView, const Position &position, GroundNeighborMap &neighbors, int x, int y)
{
    using namespace TileCoverShortHands;

    auto pos = position + Position(x, y, 0);

    // Early exits
    Tile *tilePtr = mapView.getTile(pos);
    if (!tilePtr)
    {
        return;
    }
    {
        Item *ground = tilePtr->ground();
        if (!ground)
        {
            return;
        }

        if (ground->itemType->hasFlag(ItemTypeFlag::InMountainBrush))
        {
            return;
        }
    }

    auto currentCover = neighbors.at(x, y);

    TileBorderBlock cover;
    cover.ground = currentCover.ground;

    neighbors.mirrorNorth(cover, x, y);
    neighbors.mirrorEast(cover, neighbors.at(x - 1, y));
    neighbors.mirrorSouth(cover, neighbors.at(x, y - 1));
    neighbors.mirrorWest(cover, neighbors.at(x + 1, y));

    neighbors.mirrorNorthWest(cover, neighbors.at(x - 1, y - 1));
    neighbors.mirrorNorthEast(cover, neighbors.at(x + 1, y - 1));
    neighbors.mirrorSouthEast(cover, neighbors.at(x + 1, y + 1));
    neighbors.mirrorSouthWest(cover, neighbors.at(x - 1, y + 1));

    // Do not use a mirrored diagonal if we already have a diagonal.
    for (auto &block : cover.covers)
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
    for (auto &block : cover.covers)
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

    Tile &tile = mapView.getOrCreateTile(pos);
    mapView.removeItems(tile, [](const Item &item) {
        return item.isBorder();
    });

    cover.sort();

    if (cover.covers.empty() && cover.ground && !mapView.getTile(pos)->hasGround())
    {
        cover.ground->applyWithoutBorderize(mapView, pos);
        return;
    }

    for (auto &block : cover.covers)
    {
        auto cover = block.cover;
        auto brush = block.brush;

        const auto &borderData = brush->getBorderData();

        if (brush->stackBehavior() == BorderStackBehavior::FullGround)
        {
            if (!TileCovers::exactlyOneSet(cover))
            {
                block.cover = Full;
                apply(mapView, pos, brush, BorderType::Center);
                continue;
            }
        }
        else if (brush->stackBehavior() == BorderStackBehavior::Clear)
        {
            if (!TileCovers::exactlyOneSet(cover))
            {
                block.cover = None;
                continue;
            }
        }
        else
        {
            if (cover & Full)
            {
                apply(mapView, pos, brush, BorderType::North);
                apply(mapView, pos, brush, BorderType::East);
                apply(mapView, pos, brush, BorderType::South);
                apply(mapView, pos, brush, BorderType::West);
                continue;
            }
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

    neighbors.set(x, y, cover);
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
                    if (tile.hasGround() && groundBrush == tile.ground()->itemType->getBrush(BrushType::Ground))
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
    Brush *tileGroundBrush = ground->itemType->getBrush(BrushType::Ground);
    if (tileGroundBrush)
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

BorderBrush *GroundBrush::getBorderTowards(Tile *tile, BorderAlign align) const
{
    return tile ? getBorderTowards(tile->groundBrush(), align) : nullptr;
}

BorderBrush *GroundBrush::getBorderTowards(const GroundBrush *groundBrush, BorderAlign align) const
{
    // TODO Place the default border last, otherwise it will always be picked up even if there's a more specific one.
    auto found = std::find_if(borders.begin(), borders.end(), [groundBrush, align](const GroundBorder &border) {
        return border.align == align && (!border.to || border.to->value() == groundBrush);
    });

    return (found != borders.end()) ? found->brush : nullptr;
}

BorderBrush *GroundBrush::getDefaultBorderBrush(BorderAlign align) const
{
    // TODO Place the default border last
    auto found = std::find_if(borders.begin(), borders.end(), [align](const GroundBorder &border) {
        return border.align == align && !border.to;
    });

    return (found != borders.end()) ? found->brush : nullptr;
}

const BorderCover *TileBorderBlock::border(BorderBrush *brush) const
{
    for (const auto &border : covers)
    {
        if (border.brush == brush)
            return &border;
    }

    return nullptr;
}

void TileBorderBlock::add(const BorderCover &block)
{
    add(block.cover, block.brush);
}

void TileBorderBlock::add(TileCover cover, BorderBrush *brush)
{
    if (cover == TILE_COVER_NONE)
        return;

    auto found = std::find_if(covers.begin(), covers.end(), [brush](const BorderCover &block) {
        return block.brush == brush;
    });

    if (found != covers.end())
    {
        found->cover |= cover;
    }
    else
    {
        covers.emplace_back(BorderCover(cover, brush));
    }
}

void TileBorderBlock::merge(const TileBorderBlock &other)
{
    for (const auto &border : other.covers)
    {
        add(border.cover, border.brush);
    }
}

void TileBorderBlock::sort()
{
    std::sort(covers.begin(), covers.end(), [](BorderCover &lhs, BorderCover &rhs) {
        return lhs.brush->preferredZOrder() < rhs.brush->preferredZOrder();
    });
}

uint32_t TileBorderBlock::zOrder() const noexcept
{
    return ground ? ground->zOrder() : 0;
}

const std::vector<GroundBorder> &GroundBrush::getBorders() const noexcept
{
    return borders;
}

// void validateNorth(const TileBorderBlock &center, TileBorderBlock &north)
// {
//     using namespace TileCoverShortHands;

//     for (auto &cover : north.covers)
//     {
//         BorderBrush *brush = cover.brush;

//         if (brush->centerBrush() != north.ground)
//         {
//             TileCover removeWest = SouthWestCorner;

//             TileCover removeEast = SouthEastCorner;

//             auto found = std::find_if(center.covers.begin(), center.covers.end(), [brush](const BorderCover &cover) { return cover.brush == brush; });

//             if (found != center.covers.end())
//             {
//                 TileCover reqWest = West | NorthWestCorner | SouthWest;
//                 TileCover reqEast = East | NorthEastCorner | SouthEast;

//                 if (!(found->cover & reqWest))
//                 {
//                     cover.cover &= ~removeWest;
//                 }
//                 if (!(found->cover & reqEast))
//                 {
//                     cover.cover &= ~removeEast;
//                 }
//             }
//             else
//             {
//                 cover.cover &= ~(removeWest | removeEast);
//             }
//         }
//     }
// }

void validateN(const TileBorderBlock &center, TileBorderBlock &self, TileCover req1, TileCover remove1, TileCover req2, TileCover remove2)
{
    for (auto &cover : self.covers)
    {
        BorderBrush *brush = cover.brush;

        if (brush->centerBrush() != self.ground)
        {
            auto found = std::find_if(center.covers.begin(), center.covers.end(), [brush](const BorderCover &cover) { return cover.brush == brush; });

            if (found != center.covers.end())
            {
                if (!(found->cover & req1))
                {
                    cover.cover &= ~remove1;
                }
                if (!(found->cover & req2))
                {
                    cover.cover &= ~remove2;
                }
            }
            else
            {
                cover.cover &= ~(remove1 | remove2);
            }
        }
    }
}

void GroundBrush::preBorderize(MapView &mapView, const Position &position, GroundNeighborMap &neighbors)
{
    using namespace TileCoverShortHands;
    auto &center = neighbors.center();

    // North
    validateN(center, neighbors.at(0, -1),
              West | NorthWestCorner | SouthWest, SouthWestCorner,
              East | NorthEastCorner | SouthEast, SouthEastCorner);

    // South
    validateN(center, neighbors.at(0, 1),
              West | SouthWestCorner | NorthWest, NorthWestCorner,
              East | SouthEastCorner | NorthEast, NorthEastCorner);

    // West
    validateN(center, neighbors.at(-1, 0),
              North | NorthWestCorner | NorthEast, NorthEastCorner,
              South | SouthWestCorner | SouthEast, SouthEastCorner);

    // East
    validateN(center, neighbors.at(1, 0),
              North | NorthEastCorner | NorthWest, NorthWestCorner,
              South | SouthEastCorner | SouthWest, SouthWestCorner);
}

void GroundBrush::postBorderize(MapView &mapView, const Position &position, GroundNeighborMap &neighbors)
{
    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            auto &center = neighbors.at(dx, dy);
            for (auto &cover : center.covers)
            {
                for (auto &rule : cover.brush->rules)
                {
                    const auto &otherCover = rule.check(center);
                    if (otherCover)
                    {
                        // Perform the rule cases
                        for (const auto &ruleCase : rule.cases)
                        {
                            if ((cover.cover & TileCovers::fromBorderType(ruleCase.selfEdge)) && ((*otherCover) & TileCovers::fromBorderType(ruleCase.borderEdge)))
                            {
                                Position pos = position + Position(dx, dy, 0);

                                switch (ruleCase.action->type)
                                {
                                    case BorderRuleAction::Type::Replace:
                                    {
                                        auto *action = static_cast<ReplaceAction *>(ruleCase.action.get());
                                        if (action->replaceSelf)
                                        {
                                            uint32_t oldServerId = *cover.brush->getServerId(ruleCase.selfEdge);
                                            action->apply(mapView, pos, oldServerId);
                                        }
                                        else
                                        {
                                            BorderBrush *otherBrush = Brush::getBorderBrush(rule.borderId);

                                            uint32_t oldServerId = *otherBrush->getServerId(ruleCase.borderEdge);
                                            action->apply(mapView, pos, oldServerId);
                                        }
                                        break;
                                    }
                                    case BorderRuleAction::Type::SetFull:
                                    {
                                        auto *action = static_cast<SetFullAction *>(ruleCase.action.get());
                                        BorderBrush *borderBrush = action->setSelf ? cover.brush : Brush::getBorderBrush(rule.borderId);
                                        action->apply(mapView, pos, borderBrush->centerBrush());
                                    }
                                }
                            }
                        }

                        // Perform the rule actions
                        if (!rule.actions.empty())
                        {
                            Position pos = position + Position(dx, dy, 0);

                            for (auto &action : rule.actions)
                            {
                                switch (action->type)
                                {
                                    case BorderRuleAction::Type::SetFull:
                                    {
                                        auto *setAction = static_cast<SetFullAction *>(action.get());
                                        BorderBrush *borderBrush = setAction->setSelf ? cover.brush : Brush::getBorderBrush(rule.borderId);
                                        setAction->apply(mapView, pos, borderBrush->centerBrush());
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void GroundBrush::borderize(MapView &mapView, const Position &position)
{
    using namespace TileCoverShortHands;

    GroundNeighborMap neighbors(nullptr, position, *mapView.map());

    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            auto &center = neighbors.at(dx, dy);
            for (auto &block : center.covers)
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

//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>GroundNeighborMap>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>

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
                for (auto &cover : tileCovers->covers)
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

void GroundNeighborMap::addBorderFromGround(value_type &self, const value_type &other, TileCover cover)
{
    if (!other.ground)
    {
        // Inner border from self
        if (self.ground)
        {
            BorderBrush *brush = self.ground->getBorderTowards(static_cast<GroundBrush *>(nullptr), BorderAlign::Inner);
            if (brush)
            {
                self.add(TileCovers::mirrorXY(cover), brush);
            }
        }
    }
    else
    {
        if (self.ground)
        {
            // Inner border from self
            auto innerBrush = self.ground->getBorderTowards(other.ground, BorderAlign::Inner);
            if (innerBrush)
            {
                // self.add(TileCovers::mirrorXY(cover), innerBrush);
                self.add(cover, innerBrush);
            }
            else
            {
                // Outer border from other
                if (self.zOrder() < other.zOrder())
                {
                    auto brush = other.ground->getBorderTowards(self.ground, BorderAlign::Outer);
                    if (brush)
                    {
                        self.add(cover, brush);
                    }
                }
            }
        }
    }
}

void GroundNeighborMap::mirrorNorth(TileBorderBlock &source, int dx, int dy)
{
    TileBorderBlock &borders = at(dx, dy + 1);

    uint32_t sourceZ = source.zOrder();
    for (const auto &border : borders.covers)
    {
        uint32_t z = border.brush->preferredZOrder();
        if (z > sourceZ)
        {
            TileCover cover = TileCovers::mirrorNorth(border.cover);
            source.add(cover, border.brush);
        }
    }

    addBorderFromGround(source, borders, TILE_COVER_SOUTH);
}

void GroundNeighborMap::mirrorEast(TileBorderBlock &source, const TileBorderBlock &borders)
{
    uint32_t sourceZ = source.zOrder();
    for (const auto &border : borders.covers)
    {
        uint32_t z = border.brush->preferredZOrder();
        if (z > sourceZ)
        {
            TileCover cover = TileCovers::mirrorEast(border.cover);
            source.add(cover, border.brush);
        }
    }

    addBorderFromGround(source, borders, TILE_COVER_WEST);
}

void GroundNeighborMap::mirrorSouth(value_type &source, const value_type &borders)
{
    uint32_t sourceZ = source.zOrder();
    for (const auto &border : borders.covers)
    {
        uint32_t z = border.brush->preferredZOrder();
        if (z > sourceZ)
        {
            TileCover cover = TileCovers::mirrorSouth(border.cover);
            source.add(cover, border.brush);
        }
    }

    addBorderFromGround(source, borders, TILE_COVER_NORTH);
}

void GroundNeighborMap::mirrorWest(value_type &source, const value_type &borders)
{
    uint32_t sourceZ = source.zOrder();
    for (const auto &border : borders.covers)
    {
        uint32_t z = border.brush->preferredZOrder();
        if (z > sourceZ)
        {
            TileCover cover = TileCovers::mirrorWest(border.cover);
            source.add(cover, border.brush);
        }
    }

    addBorderFromGround(source, borders, TILE_COVER_EAST);
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

    addBorderFromGround(source, borders, TILE_COVER_NORTH_WEST_CORNER);
}

void GroundNeighborMap::mirrorNorthEast(value_type &source, const value_type &borders)
{
    addBorderFromGround(source, borders, TILE_COVER_NORTH_EAST_CORNER);
}

void GroundNeighborMap::mirrorSouthEast(value_type &source, const value_type &borders)
{
    addBorderFromGround(source, borders, TILE_COVER_SOUTH_EAST_CORNER);
}

void GroundNeighborMap::mirrorSouthWest(value_type &source, const value_type &borders)
{
    addBorderFromGround(source, borders, TILE_COVER_SOUTH_WEST_CORNER);
}

BorderBrush *GroundBrush::findBorderTowards(const GroundBrush *groundBrush, BorderAlign align) const
{
    if (!groundBrush)
    {
        // Inner border from self
        BorderBrush *brush = getBorderTowards(static_cast<GroundBrush *>(nullptr), BorderAlign::Inner);
        if (brush)
        {
            return brush;
        }
    }
    else
    {
        // Inner border from self
        auto innerBrush = getBorderTowards(groundBrush, BorderAlign::Inner);
        if (innerBrush)
        {
            return innerBrush;
        }
        else
        {
            // Outer border from other
            if (zOrder() < groundBrush->zOrder())
            {
                auto brush = groundBrush->getBorderTowards(this, BorderAlign::Outer);
                if (brush)
                {
                    return brush;
                }
            }
        }
    }

    return nullptr;
}

void GroundNeighborMap::addCenterCorners()
{
    auto fix = [this](int dx, int dy, TileCover corner) {
        auto &block = at(dx, dy);
        auto brush = centerGround->findBorderTowards(block.ground, BorderAlign::Outer);

        if (brush)
        {
            block.add(corner, brush);
        }
    };

    fix(-1, -1, TILE_COVER_SOUTH_EAST_CORNER);
    fix(1, -1, TILE_COVER_SOUTH_WEST_CORNER);
    fix(-1, 1, TILE_COVER_NORTH_EAST_CORNER);
    fix(1, 1, TILE_COVER_NORTH_WEST_CORNER);
}