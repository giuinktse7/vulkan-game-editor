#include "mountain_brush.h"

#include "../items.h"
#include "../map.h"
#include "../map_view.h"
#include "../settings.h"
#include "../tile_cover.h"
#include "border_brush.h"
#include "ground_brush.h"
#include "raw_brush.h"

MountainBrush::MountainBrush(std::string id, std::string name, Brush::LazyGround ground, MountainPart::InnerWall innerWall, BorderData&& mountainBorders, uint32_t iconServerId)
    : Brush(name), _id(id), _ground(ground), innerWall(innerWall), mountainBorder(std::move(mountainBorders)), _iconServerId(iconServerId)
{
    initialize();
}

void MountainBrush::initialize()
{
    _serverIds.insert(innerWall.east);
    _serverIds.insert(innerWall.south);
    _serverIds.insert(innerWall.southEast);

    // All innerwall parts must not be set, remove the 0 created by one if it was not set
    _serverIds.erase(0);

    for (const uint32_t serverId : mountainBorder.getBorderIds())
    {
        _serverIds.insert(serverId);
    }
}

const std::unordered_set<uint32_t> &MountainBrush::serverIds() const
{
    return _serverIds;
}

void MountainBrush::apply(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);
    mapView.setGround(tile, Item(nextGroundServerId()));

    if (Settings::AUTO_BORDER)
    {
        GroundBrush::borderize(mapView, position);
        generalBorderize(mapView, position);
    }
}

void MountainBrush::applyWithoutBorderize(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);
    mapView.setGround(tile, Item(nextGroundServerId()));
}

void MountainBrush::apply(MapView &mapView, const Position &position, BorderType borderType)
{
    auto borderItemId = mountainBorder.getServerId(borderType);

    if (borderItemId && (!isFeature(TileCovers::fromBorderType(borderType)) || Settings::PLACE_MOUNTAIN_FEATURES))
    {
        mapView.addItem(position, *borderItemId);
    }
}

BorderType MountainBrush::getBorderType(uint32_t serverId) const
{
    return mountainBorder.getBorderType(serverId);
}

TileCover MountainBrush::getTileCover(uint32_t serverId) const
{
    BorderType borderType = getBorderType(serverId);
    return TileCovers::fromBorderType(borderType);
}

Brush *MountainBrush::ground() const noexcept
{
    if (!_ground.hasValue())
    {
        auto brush = _ground.value();
        for (uint32_t serverId : serverIds())
        {
            _serverIds.insert(serverId);
        }

        return brush;
    }

    return _ground.value_unsafe();
}

uint32_t MountainBrush::iconServerId() const
{
    return _iconServerId;
}

uint32_t MountainBrush::nextGroundServerId()
{
    Brush *groundBrush = ground();
    switch (groundBrush->type())
    {
        case BrushType::Raw:
            return static_cast<RawBrush *>(groundBrush)->serverId();
        case BrushType::Ground:
            return static_cast<GroundBrush *>(groundBrush)->nextServerId();
        default:
            ABORT_PROGRAM("Invalid ground brush type in MountainBrush");
    }
}

TileCover unifyTileCover(TileCover cover)
{
    using namespace TileCoverShortHands;

    TileCover clear = TILE_COVER_NONE;

    auto clearIf = [&clear, &cover](TileCover req, TileCover flags) {
        if (cover & req)
            clear |= flags;
    };

    auto replace = [&clear, &cover](TileCover a, TileCover b) {
        if ((cover & a) == a)
        {
            clear |= a;
            cover |= b;
        }
    };

    clearIf(FullNorth, NorthWestCorner | NorthEastCorner);
    clearIf(FullEast, NorthEastCorner | SouthEastCorner);
    clearIf(FullSouth, SouthWestCorner | SouthEastCorner);
    clearIf(FullWest, NorthWestCorner | SouthWestCorner);

    replace(North | West, NorthWest);
    replace(North | East, NorthEast);
    replace(South | East, SouthEast);
    replace(South | West, SouthWest);

    cover &= ~clear;
    return cover;
}

bool MountainBrush::isFeature(TileCover cover)
{
    DEBUG_ASSERT(TileCovers::exactlyOneSet(cover), "MountainBrush::isFeature: cover must be exactly one cover type.");

    return MountainBrush::Features & cover;
}

void MountainBrush::erase(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);
    mapView.removeItemsWithBorderize(tile, [this](const Item &item) { return this->erasesItem(item.serverId()); });
}

bool MountainBrush::erasesItem(uint32_t serverId) const
{
    if (Settings::PLACE_MOUNTAIN_FEATURES)
    {
        TileCover cover = TileCovers::fromBorderType(getBorderType(serverId));
        if (cover != TILE_COVER_NONE && (MountainBrush::Features & cover))
        {
            return false;
        }
    }

    return _serverIds.contains(serverId) || ground()->erasesItem(serverId);
}

void MountainBrush::addServerId(uint32_t serverId)
{
    _serverIds.emplace(serverId);
}

BrushType MountainBrush::type() const
{
    return BrushType::Mountain;
}

std::vector<ThingDrawInfo> MountainBrush::getPreviewTextureInfo(int variation) const
{
    return ground()->getPreviewTextureInfo(variation);
}

const std::string MountainBrush::getDisplayId() const
{
    return _id;
}

const std::string &MountainBrush::id() const noexcept
{
    return _id;
}

void MountainBrush::setGround(GroundBrush *brush)
{
    this->_ground = Brush::LazyGround(brush);
}

void MountainBrush::setGround(const std::string &groundBrushId)
{
    this->_ground = Brush::LazyGround(groundBrushId);
}

bool MountainPart::InnerWall::contains(uint32_t serverId) const noexcept
{
    return south == serverId || east == serverId || southEast == serverId;
}

bool MountainBrush::isFeature(const ItemType &itemType) const
{
    uint32_t serverId = itemType.id;
    return mountainBorder.getBorderType(serverId) != BorderType::None;
}

void MountainPart::BorderBlock::add(const MountainCover &block)
{
    add(block.cover, block.brush);
}

void MountainPart::BorderBlock::add(TileCover cover, MountainBrush *brush)
{
    auto found = std::find_if(covers.begin(), covers.end(), [brush](const MountainCover &block) {
        return block.brush == brush;
    });

    if (found != covers.end())
    {
        found->cover |= cover;
    }
    else
    {
        covers.emplace_back(MountainCover(cover, brush));
    }
}

void MountainPart::BorderBlock::merge(const BorderBlock &other)
{
    for (const auto &border : other.covers)
    {
        add(border.cover, border.brush);
    }
}

void MountainPart::BorderBlock::sort()
{
    std::sort(covers.begin(), covers.end(), [](MountainCover &lhs, MountainCover &rhs) {
        // TODO
        int lhsPreferredZOrder = 0;
        int rhsPreferredZOrder = 0;
        return lhsPreferredZOrder < rhsPreferredZOrder;
    });
}

const MountainPart::MountainCover *MountainPart::BorderBlock::getCover(MountainBrush *brush) const
{
    for (const auto &cover : covers)
    {
        if (cover.brush == brush)
        {
            return &cover;
        }
    }

    return nullptr;
}

void MountainBrush::generalBorderize(MapView &mapView, const Position &position)
{
    using namespace TileCoverShortHands;

    MountainNeighborMap neighbors(position, *mapView.map());

    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            auto &center = neighbors.at(dx, dy);
            for (auto &block : center.covers)
            {
                TileCover featureRemove = None;

#define fast_removeInvalidBorder(_x, _y, _requiredCover, _removeCover)            \
    do                                                                            \
    {                                                                             \
        auto neighbor = neighbors.at(dx + (_x), dy + (_y)).getCover(block.brush); \
        if (!(neighbor && (neighbor->cover & (_requiredCover))))                  \
        {                                                                         \
            featureRemove |= (_removeCover);                                      \
        }                                                                         \
    } while (false)

#define fast_eraseInvalidSide(_x, _y, _requiredCover, _side)                      \
    do                                                                            \
    {                                                                             \
        auto neighbor = neighbors.at(dx + (_x), dy + (_y)).getCover(block.brush); \
        if (!(neighbor && (neighbor->cover & (_requiredCover))))                  \
        {                                                                         \
            TileCovers::eraseSide(block.cover, (_side));                          \
        }                                                                         \
    } while (false)

                fast_eraseInvalidSide(0, -1, FullSouth, North);
                fast_eraseInvalidSide(1, 0, FullWest, East);
                fast_eraseInvalidSide(0, 1, FullNorth, South);
                fast_eraseInvalidSide(-1, 0, FullEast, West);

                fast_removeInvalidBorder(-1, -1, Full | NorthEast | SouthWest | East | South | SouthEastCorner, NorthWestCorner);
                fast_removeInvalidBorder(1, -1, Full | NorthWest | SouthEast | West | South | SouthWestCorner, NorthEastCorner);
                fast_removeInvalidBorder(1, 1, Full | NorthWest | SouthWest | West | North | NorthWestCorner, SouthEastCorner);
                fast_removeInvalidBorder(-1, 1, Full | NorthWest | SouthEast | West | North | NorthEastCorner, SouthWestCorner);

                if (featureRemove != None)
                {
                    block.cover &= ~featureRemove;
                }
            }
        }
    }

    fixBorders(mapView, position, neighbors);
}

void MountainBrush::fixBorders(MapView &mapView, const Position &position, MountainNeighborMap &neighbors)
{
    // Place north and west first so that mirroring works
    fixBordersAtOffset(mapView, position, neighbors, 0, -1);
    fixBordersAtOffset(mapView, position, neighbors, -1, 0);

    // Top
    fixBordersAtOffset(mapView, position, neighbors, -1, -1);
    fixBordersAtOffset(mapView, position, neighbors, 1, -1);

    // Middle
    fixBordersAtOffset(mapView, position, neighbors, 0, 0);
    fixBordersAtOffset(mapView, position, neighbors, 1, 0);

    // Bottom
    fixBordersAtOffset(mapView, position, neighbors, -1, 1);
    fixBordersAtOffset(mapView, position, neighbors, 0, 1);
    fixBordersAtOffset(mapView, position, neighbors, 1, 1);
}

void MountainBrush::fixBordersAtOffset(MapView &mapView, const Position &position, MountainNeighborMap &neighbors, int x, int y)
{
    using namespace TileCoverShortHands;

    auto pos = position + Position(x, y, 0);

    if (mapView.hasTile(pos))
    {
        // Remove current mountain borders on the tile
        mapView.removeItems(*mapView.getTile(pos), [](const Item &item) {
            return item.itemType->hasFlag(ItemTypeFlag::InMountainBrush);
        });
    }

    auto currentCover = neighbors.at(x, y);

    if (currentCover.ground)
    {
        // Make sure the tile is created
        if (!mapView.hasTile(pos))
        {
            mapView.createTile(pos);
        }

        currentCover.ground->ground()->applyWithoutBorderize(mapView, pos);

        bool innerEast = !neighbors.at(x + 1, y).ground;
        bool innerSouth = !neighbors.at(x, y + 1).ground;

        if (innerEast && innerSouth)
        {
            mapView.addItem(pos, currentCover.ground->innerWall.southEast);
        }
        else if (innerEast)
        {
            mapView.addItem(pos, currentCover.ground->innerWall.east);
        }
        else if (innerSouth)
        {
            mapView.addItem(pos, currentCover.ground->innerWall.south);
        }

        return;
    }

    // Early exits
    Tile &tile = mapView.getOrCreateTile(pos);
    {
        Item *ground = tile.ground();
        if (ground && ground->itemType->hasFlag(ItemTypeFlag::InMountainBrush))
        {
            return;
        }
    }

    MountainPart::BorderBlock borderBlock;
    borderBlock.ground = currentCover.ground;

    MountainNeighborMap::mirrorNorth(borderBlock, neighbors.at(x, y + 1));
    MountainNeighborMap::mirrorEast(borderBlock, neighbors.at(x - 1, y));
    MountainNeighborMap::mirrorSouth(borderBlock, neighbors.at(x, y - 1));
    MountainNeighborMap::mirrorWest(borderBlock, neighbors.at(x + 1, y));

    // MountainNeighborMap::mirrorNorthWest(borderBlock, neighbors.at(x - 1, y - 1));
    // MountainNeighborMap::mirrorNorthEast(borderBlock, neighbors.at(x + 1, y - 1));
    // MountainNeighborMap::mirrorSouthEast(borderBlock, neighbors.at(x + 1, y + 1));
    // MountainNeighborMap::mirrorSouthWest(borderBlock, neighbors.at(x - 1, y + 1));

    // Do not use a mirrored diagonal if we already have a diagonal.
    for (auto &block : borderBlock.covers)
    {
        auto current = currentCover.getCover(block.brush);
        if (current && current->cover & Diagonals)
        {
            block.cover &= ~(Diagonals);
        }
    }

    borderBlock.merge(currentCover);

    // auto quadrant = mapView.getMouseDownTileQuadrant();

    // // TileQuadrant quadrant = getNeighborQuadrant(x, y);
    for (auto &cover : borderBlock.covers)
    {
        //     // Compute preferred diagonal
        //     TileCover preferredDiagonal = block.cover & Diagonals;
        //     if (x == 0 && y == 0)
        //     {
        //         if (quadrant)
        //         {
        //             switch (*quadrant)
        //             {
        //                 case TileQuadrant::TopLeft:
        //                     preferredDiagonal = NorthWest;
        //                     break;
        //                 case TileQuadrant::TopRight:
        //                     preferredDiagonal = NorthEast;
        //                     break;
        //                 case TileQuadrant::BottomRight:
        //                     preferredDiagonal = SouthEast;
        //                     break;
        //                 case TileQuadrant::BottomLeft:
        //                     preferredDiagonal = SouthWest;
        //                     break;
        //             }
        //         }
        //     }
        cover.cover = TileCovers::unifyTileCover(cover.cover, TileQuadrant::BottomRight);

        // Special cases
        if (!Settings::PLACE_MOUNTAIN_FEATURES)
        {
            TileCover &featureCover = cover.cover;
            // cover &= ~MountainBrush::features;

            if ((featureCover & SouthWest) && !(neighbors.at(x + 1, y + 1).ground || neighbors.at(x + 1, y).ground))
            {
                featureCover &= ~SouthWest;
                if (!(featureCover & FullSouth))
                {
                    featureCover |= South;
                }
            }

            if ((featureCover & NorthEast) && !(neighbors.at(x + 1, y + 1).ground || neighbors.at(x, y + 1).ground))
            {
                featureCover &= ~NorthEast;
                if (!(featureCover & FullEast))
                {
                    featureCover |= East;
                }
            }
        }
    }

    neighbors.set(x, y, borderBlock);

    for (const auto &block : borderBlock.covers)
    {
        auto cover = block.cover;
        auto brush = block.brush;

        if (cover & Full)
        {
            applyFeature(mapView, pos, brush, BorderType::NorthWestDiagonal);
            applyFeature(mapView, pos, brush, BorderType::SouthEastDiagonal);
            return;
        }

        auto applyIf = [&cover, &mapView, &pos, &brush](TileCover req, BorderType borderType) {
            if (cover & req)
            {
                applyFeature(mapView, pos, brush, borderType);
            }
        };

        // Sides
        applyIf(North, BorderType::North);
        applyIf(West, BorderType::West);

        applyIf(NorthWest, BorderType::NorthWestDiagonal);

        // Corners
        if (cover & Corners)
        {
            applyIf(NorthEastCorner, BorderType::NorthEastCorner);
            applyIf(NorthWestCorner, BorderType::NorthWestCorner);
            applyIf(SouthWestCorner, BorderType::SouthWestCorner);
        }

        // Diagonals
        if (cover & Diagonals)
        {
            applyIf(NorthEast, BorderType::NorthEastDiagonal);
            applyIf(SouthWest, BorderType::SouthWestDiagonal);
            applyIf(SouthEast, BorderType::SouthEastDiagonal);
        }

        applyIf(East, BorderType::East);
        applyIf(South, BorderType::South);

        applyIf(SouthEastCorner, BorderType::SouthEastCorner);
    }
}

uint32_t MountainPart::BorderBlock::zOrder() const noexcept
{
    // TODO
    return 0;
}

void MountainBrush::applyFeature(MapView &mapView, const Position &position, MountainBrush *brush, BorderType borderType)
{
    auto borderItemId = brush->mountainBorder.getServerId(borderType);

    if (borderItemId && (!isFeature(TileCovers::fromBorderType(borderType)) || Settings::PLACE_MOUNTAIN_FEATURES))
    {
        mapView.addItem(position, *borderItemId);
    }
}

MountainPart::MountainCover::MountainCover(TileCover cover, MountainBrush *brush)
    : cover(cover), brush(brush) {}

// >>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>
// >>>MountainNeighborMap>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>

MountainNeighborMap::MountainNeighborMap(const Position &position, const Map &map)
{
    TileCover noFeatureMask = ~MountainBrush::Features;
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            auto borderBlock = getTileCoverAt(map, position + Position(dx, dy, 0));
            if (borderBlock)
            {
                for (auto &cover : borderBlock->covers)
                {
                    cover.cover = TileCovers::unifyTileCover(cover.cover, TileQuadrant::TopLeft);

                    if (!Settings::PLACE_MOUNTAIN_FEATURES)
                    {
                        cover.cover &= noFeatureMask;
                    }
                }
                set(dx, dy, *borderBlock);
            }
        }
    }
}

bool MountainNeighborMap::isMountainFeaturePart(const ItemType &itemType) const
{
    Brush *brush = itemType.getBrush(BrushType::Mountain);
    if (!brush)
    {
        return false;
    }

    return static_cast<MountainBrush *>(brush)->isFeature(itemType);
}

MountainPart::BorderBlock &MountainNeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

MountainPart::BorderBlock &MountainNeighborMap::center()
{
    return data[12];
}

MountainPart::BorderBlock MountainNeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

void MountainNeighborMap::set(int x, int y, MountainPart::BorderBlock tileCover)
{
    data[index(x, y)] = tileCover;
}

int MountainNeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}

std::optional<MountainPart::BorderBlock> MountainNeighborMap::getTileCoverAt(const Map &map, const Position position) const
{
    Tile *tile = map.getTile(position);

    if (!tile)
    {
        return std::nullopt;
    }

    MountainPart::BorderBlock block;
    if (tile->hasGround())
    {
        Brush *brush = tile->ground()->itemType->getBrush(BrushType::Mountain);
        if (brush)
        {
            auto *groundBrush = static_cast<MountainBrush *>(brush);
            block.ground = groundBrush;
            block.add(TILE_COVER_FULL, groundBrush);
            return block;
        }
    }

    for (const auto &item : tile->items())
    {
        const ItemType &itemType = *item->itemType;
        if (isMountainFeaturePart(itemType))
        {
            auto mountainBrush = static_cast<MountainBrush *>(itemType.brush());
            TileCover cover = mountainBrush->getTileCover(item->serverId());
            if (cover != TILE_COVER_NONE)
            {
                block.add(cover, mountainBrush);
            }
        }
    }

    return block;
}

void MountainNeighborMap::mirrorNorth(value_type &source, const value_type &borders)
{
    if (borders.ground)
    {
        TileCover cover = TileCovers::mirrorNorth(TILE_COVER_FULL);
        source.add(cover, borders.ground);
        return;
    }

    uint32_t sourceZ = source.zOrder();
    for (const auto &cover : borders.covers)
    {
        // uint32_t z = cover.brush->preferredZOrder();
        uint32_t z = 10000;
        if (z > sourceZ)
        {
            source.add(TileCovers::mirrorNorth(cover.cover), cover.brush);
        }
    }

    // addGroundBorder(source, borders, TILE_COVER_SOUTH);
}

void MountainNeighborMap::mirrorEast(value_type &source, const value_type &borders)
{
    if (borders.ground)
    {
        TileCover cover = TileCovers::mirrorEast(TILE_COVER_FULL);
        source.add(cover, borders.ground);
        return;
    }

    uint32_t sourceZ = source.zOrder();
    for (const auto &cover : borders.covers)
    {
        // uint32_t z = cover.brush->preferredZOrder();
        uint32_t z = 10000;
        if (z > sourceZ)
        {

            source.add(TileCovers::mirrorEast(cover.cover), cover.brush);
        }
    }

    // addGroundBorder(source, borders, TILE_COVER_SOUTH);
}

void MountainNeighborMap::mirrorSouth(value_type &source, const value_type &borders)
{
    if (borders.ground)
    {
        TileCover cover = TileCovers::mirrorSouth(TILE_COVER_FULL);
        source.add(cover, borders.ground);
        return;
    }

    uint32_t sourceZ = source.zOrder();
    for (const auto &cover : borders.covers)
    {
        // uint32_t z = cover.brush->preferredZOrder();
        uint32_t z = 10000;
        if (z > sourceZ)
        {
            source.add(TileCovers::mirrorSouth(cover.cover), cover.brush);
        }
    }

    // addGroundBorder(source, borders, TILE_COVER_SOUTH);
}

void MountainNeighborMap::mirrorWest(value_type &source, const value_type &borders)
{
    if (borders.ground)
    {
        TileCover cover = TileCovers::mirrorWest(TILE_COVER_FULL);
        source.add(cover, borders.ground);
        return;
    }

    uint32_t sourceZ = source.zOrder();
    for (const auto &cover : borders.covers)
    {
        // uint32_t z = cover.brush->preferredZOrder();
        uint32_t z = 10000;
        if (z > sourceZ)
        {

            source.add(TileCovers::mirrorWest(cover.cover), cover.brush);
        }
    }
}

void MountainNeighborMap::mirrorNorthWest(value_type &source, const value_type &borders)
{
}

void MountainNeighborMap::mirrorNorthEast(value_type &source, const value_type &borders)
{
}

void MountainNeighborMap::mirrorSouthEast(value_type &source, const value_type &borders)
{
    using namespace TileCoverShortHands;

    if (borders.ground)
    {
        source.add(TILE_COVER_SOUTH_EAST_CORNER, borders.ground);
        return;
    }

    uint32_t sourceZ = source.zOrder();
    for (const auto &cover : borders.covers)
    {
        // uint32_t z = cover.brush->preferredZOrder();
        uint32_t z = 99999;
        if (z > sourceZ && (cover.cover & (Full | NorthWest | SouthWest | West | North | NorthWestCorner)))
        {
            source.add(TILE_COVER_SOUTH_EAST_CORNER, cover.brush);
        }
    }
}

void MountainNeighborMap::mirrorSouthWest(value_type &source, const value_type &borders)
{
}
