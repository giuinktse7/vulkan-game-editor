#include "mountain_brush.h"

#include "../items.h"
#include "../map.h"
#include "../map_view.h"
#include "../settings.h"
#include "../tile_cover.h"
#include "border_brush.h"
#include "ground_brush.h"
#include "raw_brush.h"

MountainBrush::MountainBrush(std::string id, std::string name, Brush::LazyGround ground, MountainPart::InnerWall innerWall, BorderData mountainBorders, uint32_t iconServerId)
    : Brush(name), _id(id), _ground(ground), innerWall(innerWall), mountainBorder(mountainBorders), _iconServerId(iconServerId)
{
    initialize();
}

MountainBrush::MountainBrush(std::string id, std::string name, Brush::LazyGround ground, MountainPart::InnerWall innerWall, BorderData mountainBorders, std::string outerBorderBrushId, uint32_t iconServerId)
    : Brush(name), _id(id), _ground(ground), innerWall(innerWall), mountainBorder(mountainBorders), _iconServerId(iconServerId)
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

void MountainBrush::setOuterBorder(std::string outerBorderId)
{
    this->outerBorder = std::make_optional<LazyObject<BorderBrush *>>([outerBorderId]() {
        BorderBrush *brush = Brush::getBorderBrush(outerBorderId);
        if (!brush)
        {
            ABORT_PROGRAM(std::format("Attempted to retrieve a GroundBrush with id '{}' from a MountainBrush::outerBorder, but the brush did not exist.", outerBorderId));
        }

        return brush;
    });
}

const std::unordered_set<uint32_t> &MountainBrush::serverIds() const
{
    return _serverIds;
}

void MountainBrush::apply(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);
    mapView.setGround(tile, Item(nextGroundServerId()));

    generalBorderize(mapView, position);

    // if (Settings::AUTO_BORDER)
    // {
    //     MountainGroundNeighborMap neighbors = MountainGroundNeighborMap(_ground.value(), position, *mapView.map());

    //     for (int dy = -1; dy <= 1; ++dy)
    //     {
    //         for (int dx = -1; dx <= 1; ++dx)
    //         {
    //             borderize(mapView, neighbors, dx, dy);
    //         }
    //     }
    // }

    // postBorderizePass(mapView, position);
}

// void MountainBrush::postBorderizePass(MapView &mapView, const Position &position)
// {
//     using namespace TileCoverShortHands;
//     auto neighbors = MountainWallNeighborMap(this, position, *mapView.map());

//     for (int dy = -2; dy <= 2; ++dy)
//     {
//         for (int dx = -2; dx <= 2; ++dx)
//         {
//             Position pos = position + Position(dx, dy, 0);
//             TileCover &center = neighbors.at(dx, dy);
//             if ((center & SouthWest) && dx < 2)
//             {
//                 TileCover east = neighbors.at(dx + 1, dy);
//                 if (!(east & (SouthEast | South | SouthWestCorner)))
//                 {
//                     center &= ~SouthWest;
//                     center |= South;
//                     center |= West;

//                     Tile *tile = mapView.getTile(pos);
//                     mapView.removeItems(*tile, [this](const Item &item) {
//                         return item.serverId() == mountainBorders.getServerId(BorderType::SouthWestDiagonal);
//                     });

//                     apply(mapView, pos, BorderType::West);
//                     apply(mapView, pos, BorderType::South);
//                 }
//             }
//         }
//     }
// }

void MountainBrush::apply(MapView &mapView, const Position &position, BorderType borderType)
{
    auto borderItemId = mountainBorder.getServerId(borderType);

    if (outerBorder && borderType != BorderType::Center)
    {
        Tile *tile = mapView.getTile(position);
        if (tile && tile->hasGround())
        {
            outerBorder->value()->apply(mapView, position, borderType);
        }
    }

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

TileCover MountainBrush::getBorderTileCover(uint32_t serverId) const
{
    BorderType borderType = outerBorder->value()->getBorderType(serverId);
    return TileCovers::fromBorderType(borderType);
}

Brush *MountainBrush::ground() const noexcept
{
    if (!_ground.hasValue())
    {
        auto brush = _ground.value();
        switch (brush->type())
        {
            case BrushType::Raw:
                _serverIds.insert(static_cast<RawBrush *>(brush)->serverId());
                break;
            case BrushType::Ground:
                for (uint32_t serverId : static_cast<GroundBrush *>(brush)->serverIds())
                {
                    _serverIds.insert(serverId);
                }
                break;
            default:
                break;
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

void MountainBrush::borderize(MapView &mapView, const Position &position)
{
    MountainGroundNeighborMap neighbors = MountainGroundNeighborMap(_ground.value(), position, *mapView.map());

    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            borderize(mapView, neighbors, dx, dy);
        }
    }
}

void MountainBrush::borderize(MapView &mapView, MountainGroundNeighborMap &neighbors, int dx, int dy)
{
    using namespace TileCoverShortHands;

    const auto &center = neighbors.at(dx, dy);

    Position pos = neighbors.position + Position(dx, dy, 0);

    if (mapView.hasTile(pos))
    {
        // Remove current mountain borders on the tile
        mapView.removeItems(pos, [this](const Item &item) {
            uint32_t serverId = item.serverId();
            return erasesItem(serverId) && !ground()->erasesItem(serverId); });
    }

    if (center.hasMountainGround)
    {
        // Make sure the tile is created
        if (!mapView.hasTile(pos))
        {
            mapView.createTile(pos);
        }

        bool innerEast = !neighbors.at(dx + 1, dy).hasMountainGround;
        bool innerSouth = !neighbors.at(dx, dy + 1).hasMountainGround;

        if (innerEast && innerSouth)
        {
            mapView.addItem(pos, innerWall.southEast);
        }
        else if (innerEast)
        {
            mapView.addItem(pos, innerWall.east);
        }
        else if (innerSouth)
        {
            mapView.addItem(pos, innerWall.south);
        }
    }
    else
    {
        TileCover cover = neighbors.getTileCover(dx, dy);
        cover = unifyTileCover(cover);

        // Special cases
        if (!Settings::PLACE_MOUNTAIN_FEATURES)
        {
            // cover &= ~MountainBrush::features;

            if ((cover & SouthWest) && !(neighbors.at(dx + 1, dy + 1).hasMountainGround || neighbors.at(dx + 1, dy).hasMountainGround))
            {
                cover &= ~SouthWest;
                if (!(cover & FullSouth))
                {
                    cover |= South;
                }
            }

            if ((cover & NorthEast) && !(neighbors.at(dx + 1, dy + 1).hasMountainGround || neighbors.at(dx, dy + 1).hasMountainGround))
            {
                cover &= ~NorthEast;
                if (!(cover & FullEast))
                {
                    cover |= East;
                }
            }
        }

        bool hasFeature = false;

        auto applyIf = [this, &cover, &mapView, &pos, &hasFeature](TileCover req, BorderType borderType, bool feature = false) {
            if (cover & req)
            {
                if (feature)
                {
                    if (!hasFeature)
                    {
                        apply(mapView, pos, borderType);
                    }

                    hasFeature = true;
                }
                else
                {
                    apply(mapView, pos, borderType);
                }
            }
        };

        // Sides
        applyIf(North, BorderType::North, true);
        applyIf(West, BorderType::West, true);

        applyIf(NorthWest, BorderType::NorthWestDiagonal, true);

        // Corners
        if (cover & Corners)
        {
            applyIf(NorthEastCorner, BorderType::NorthEastCorner, true);
            applyIf(NorthWestCorner, BorderType::NorthWestCorner, true);
            applyIf(SouthWestCorner, BorderType::SouthWestCorner, true);
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
    return _serverIds.contains(serverId) || (outerBorder && outerBorder.value().value()->erasesItem(serverId));
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

void MountainBrush::setGround(RawBrush *brush)
{
    this->_ground = Brush::LazyGround(brush);
}

void MountainBrush::setGround(GroundBrush *brush)
{
    this->_ground = Brush::LazyGround(brush);
}

void MountainBrush::setGround(const std::string &groundBrushId)
{
    this->_ground = Brush::LazyGround(groundBrushId);
}

MountainWallNeighborMap::MountainWallNeighborMap(MountainBrush *mountainBrush, const Position &position, const Map &map)
    : position(position)
{
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            Tile *tile = map.getTile(position + Position(dx, dy, 0));
            if (!tile)
            {
                set(dx, dy, TILE_COVER_NONE);
            }
            else
            {
                TileCover cover = tile->getTileCover(mountainBrush);
                cover = unifyTileCover(cover);
                set(dx, dy, cover);
            }
        }
    }
}

TileCover MountainWallNeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

TileCover &MountainWallNeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

void MountainWallNeighborMap::set(int x, int y, TileCover cover)
{
    data[index(x, y)] = cover;
}

int MountainWallNeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}

MountainGroundNeighborMap::MountainGroundNeighborMap(Brush *ground, const Position &position, const Map &map)
    : position(position), ground(ground)
{
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            Tile *tile = map.getTile(position + Position(dx, dy, 0));
            Entry entry;

            entry.hasMountainGround = tile && tile->hasGround() && ground->erasesItem(tile->ground()->itemType->id);
            set(dx, dy, entry);
        }
    }
}

void MountainGroundNeighborMap::set(int x, int y, value_type entry)
{
    data[index(x, y)] = entry;
}

bool MountainPart::InnerWall::contains(uint32_t serverId) const noexcept
{
    return south == serverId || east == serverId || southEast == serverId;
}

MountainGroundNeighborMap::value_type MountainGroundNeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

MountainGroundNeighborMap::value_type &MountainGroundNeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

int MountainGroundNeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}

TileCover MountainGroundNeighborMap::getTileCover(int dx, int dy) const noexcept
{
    using namespace TileCoverShortHands;

    TileCover cover = TILE_COVER_NONE;

    auto includeIfGround = [this, dx, dy, &cover](int offsetX, int offsetY, TileCover part) {
        if (at(dx + offsetX, dy + offsetY).hasMountainGround)
        {
            cover |= part;
        }
    };

    includeIfGround(-1, -1, NorthWestCorner);
    includeIfGround(0, -1, North);
    includeIfGround(1, -1, NorthEastCorner);

    includeIfGround(-1, 0, West);
    includeIfGround(1, 0, East);

    includeIfGround(-1, 1, SouthWestCorner);
    includeIfGround(0, 1, South);
    includeIfGround(1, 1, SouthEastCorner);

    return cover;
}

// >>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>
// >>>MountainNeighborMap>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>
// >>>>>>>>>>>>>>>>>>>>>>>>>

MountainNeighborMap::MountainNeighborMap(const Position &position, const Map &map)
{
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            auto borderBlock = getTileCoverAt(map, position + Position(dx, dy, 0));
            if (borderBlock)
            {
                for (auto &cover : borderBlock->covers)
                {
                    cover.featureCover = TileCovers::unifyTileCover(cover.featureCover, TileQuadrant::TopLeft);
                    cover.borderCover = TileCovers::unifyTileCover(cover.borderCover, TileQuadrant::TopLeft);

                    if (!Settings::PLACE_MOUNTAIN_FEATURES)
                    {
                        cover.featureCover &= ~MountainBrush::Features;
                    }
                }
                set(dx, dy, *borderBlock);
            }
        }
    }
}

bool MountainBrush::isFeature(const ItemType &itemType) const
{
    uint32_t serverId = itemType.id;
    return mountainBorder.getBorderType(serverId) != BorderType::None;
}

bool MountainBrush::isOuterBorder(const ItemType &itemType) const
{
    uint32_t serverId = itemType.id;
    return outerBorder && outerBorder->value()->includes(serverId);
}

bool MountainNeighborMap::isMountainFeaturePart(const ItemType &itemType) const
{
    Brush *brush = itemType.brush;
    if (!brush || brush->type() != BrushType::Mountain)
    {
        return false;
    }

    return static_cast<MountainBrush *>(brush)->isFeature(itemType);
}

bool MountainNeighborMap::isMountainOuterBorderPart(const ItemType &itemType) const
{
    Brush *brush = itemType.brush;
    if (!brush || brush->type() != BrushType::Mountain)
    {
        return false;
    }

    return static_cast<MountainBrush *>(brush)->isOuterBorder(itemType);
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
        Brush *brush = tile->ground()->itemType->brush;
        if (brush && brush->type() == BrushType::Mountain)
        {
            auto *groundBrush = static_cast<MountainBrush *>(brush);
            block.ground = groundBrush;
            block.addFeature(TILE_COVER_FULL, groundBrush);
        }
    }

    for (const auto &item : tile->items())
    {
        const ItemType &itemType = *item->itemType;
        if (isMountainFeaturePart(itemType))
        {
            auto mountainBrush = static_cast<MountainBrush *>(itemType.brush);
            TileCover cover = mountainBrush->getTileCover(item->serverId());
            if (cover != TILE_COVER_NONE)
            {
                block.addFeature(cover, mountainBrush);
            }
        }
        else if (isMountainOuterBorderPart(itemType))
        {
            auto mountainBrush = static_cast<MountainBrush *>(itemType.brush);
            TileCover cover = mountainBrush->getBorderTileCover(item->serverId());
            if (cover != TILE_COVER_NONE)
            {
                block.addBorder(cover, mountainBrush);
            }
        }
    }

    return block;
}

void MountainPart::BorderBlock::add(const MountainCover &block)
{
    add(block.featureCover, block.borderCover, block.brush);
}

void MountainPart::BorderBlock::addFeature(TileCover cover, MountainBrush *brush)
{
    add(cover, TILE_COVER_NONE, brush);
}

void MountainPart::BorderBlock::addBorder(TileCover cover, MountainBrush *brush)
{
    add(TILE_COVER_NONE, cover, brush);
}

void MountainPart::BorderBlock::add(TileCover cover, MountainBrush *brush)
{
    add(cover, cover, brush);
}

void MountainPart::BorderBlock::add(TileCover featureCover, TileCover borderCover, MountainBrush *brush)
{

    auto found = std::find_if(covers.begin(), covers.end(), [brush](const MountainCover &block) {
        return block.brush == brush;
    });

    if (found != covers.end())
    {
        found->featureCover |= featureCover;
        found->borderCover |= borderCover;
    }
    else
    {
        covers.emplace_back(MountainCover(featureCover, borderCover, brush));
    }
}

void MountainPart::BorderBlock::merge(const BorderBlock &other)
{
    for (const auto &border : other.covers)
    {
        add(border.featureCover, border.borderCover, border.brush);
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
                TileCover borderRemove = None;

#define fast_removeInvalidBorder(_x, _y, _requiredCover, _removeCover)            \
    do                                                                            \
    {                                                                             \
        auto neighbor = neighbors.at(dx + (_x), dy + (_y)).getCover(block.brush); \
        if (!(neighbor && (neighbor->featureCover & (_requiredCover))))           \
        {                                                                         \
            featureRemove |= (_removeCover);                                      \
        }                                                                         \
        if (!(neighbor && (neighbor->borderCover & (_requiredCover))))            \
        {                                                                         \
            borderRemove |= (_removeCover);                                       \
        }                                                                         \
    } while (false)

#define fast_eraseInvalidSide(_x, _y, _requiredCover, _side)                      \
    do                                                                            \
    {                                                                             \
        auto neighbor = neighbors.at(dx + (_x), dy + (_y)).getCover(block.brush); \
        if (!(neighbor && (neighbor->featureCover & (_requiredCover))))           \
        {                                                                         \
            TileCovers::eraseSide(block.featureCover, (_side));                   \
            TileCovers::eraseSide(block.borderCover, (_side));                    \
        }                                                                         \
    } while (false)

                // const auto removeInvalidBorder = [&neighbors, &block, &featureRemove, &borderRemove, dx, dy](int x, int y, TileCover requiredCover, TileCover removeCover) {
                //     auto neighbor = neighbors.at(dx + x, dy + y).getCover(block.brush);
                //     if (!(neighbor && (neighbor->featureCover & requiredCover)))
                //     {
                //         featureRemove |= removeCover;
                //     }
                //     if (!(neighbor && (neighbor->borderCover & requiredCover)))
                //     {
                //         borderRemove |= removeCover;
                //     }
                // };

                // const auto eraseInvalidSide = [&neighbors, &block, dx, dy](int x, int y, TileCover requirement, TileCover side) {
                //     auto neighbor = neighbors.at(dx + x, dy + y).getCover(block.brush);
                //     if (!(neighbor && (neighbor->featureCover & requirement)))
                //     {
                //         TileCovers::eraseSide(block.featureCover, side);
                //         TileCovers::eraseSide(block.borderCover, side);
                //     }
                // };

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
                    block.featureCover &= ~featureRemove;
                }

                if (borderRemove != None)
                {
                    block.borderCover &= ~borderRemove;
                }
            }
        }
    }

    fixBorders(mapView, position, neighbors);
}

void MountainBrush::fixBorders(MapView &mapView, const Position &position, MountainNeighborMap &neighbors)
{
    // Top
    fixBordersAtOffset(mapView, position, neighbors, -1, -1);
    fixBordersAtOffset(mapView, position, neighbors, 0, -1);
    fixBordersAtOffset(mapView, position, neighbors, 1, -1);

    // Middle
    fixBordersAtOffset(mapView, position, neighbors, -1, 0);
    fixBordersAtOffset(mapView, position, neighbors, 0, 0);
    fixBordersAtOffset(mapView, position, neighbors, 1, 0);

    // Bottom
    fixBordersAtOffset(mapView, position, neighbors, -1, 1);
    fixBordersAtOffset(mapView, position, neighbors, 0, 1);
    fixBordersAtOffset(mapView, position, neighbors, 1, 1);
}

void MountainBrush::applyGround(MapView &mapView, const Position &position)
{
    Brush *groundBrush = ground();
    switch (groundBrush->type())
    {
        case BrushType::Raw:
            static_cast<RawBrush *>(groundBrush)->apply(mapView, position);
            break;
        case BrushType::Ground:
            static_cast<GroundBrush *>(groundBrush)->applyWithoutBordering(mapView, position);
            break;
        default:
            ABORT_PROGRAM("Incorrect brush type in MountainBrush::applyGround");
    }
}

void MountainBrush::fixBordersAtOffset(MapView &mapView, const Position &position, MountainNeighborMap &neighbors, int x, int y)
{
    using namespace TileCoverShortHands;

    auto pos = position + Position(x, y, 0);

    if (mapView.hasTile(pos))
    {
        // Remove current mountain borders on the tile
        mapView.removeItems(*mapView.getTile(pos), [](const Item &item) {
            Brush *brush = item.itemType->brush;
            return brush && brush->type() == BrushType::Mountain;
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

        currentCover.ground->applyGround(mapView, pos);

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
        if (ground)
        {
            Brush *groundBrush = ground->itemType->brush;
            bool isMountain = groundBrush && groundBrush->type() == BrushType::Mountain;
            if (isMountain)
            {
                return;
            }
        }
    }

    MountainPart::BorderBlock borderBlock;
    borderBlock.ground = currentCover.ground;

    MountainNeighborMap::mirrorNorth(borderBlock, neighbors.at(x, y + 1));
    MountainNeighborMap::mirrorEast(borderBlock, neighbors.at(x - 1, y));
    MountainNeighborMap::mirrorSouth(borderBlock, neighbors.at(x, y - 1));
    MountainNeighborMap::mirrorWest(borderBlock, neighbors.at(x + 1, y));

    MountainNeighborMap::mirrorNorthWest(cover, neighbors.at(x - 1, y - 1));
    MountainNeighborMap::mirrorNorthEast(cover, neighbors.at(x + 1, y - 1));
    MountainNeighborMap::mirrorSouthEast(cover, neighbors.at(x + 1, y + 1));
    MountainNeighborMap::mirrorSouthWest(cover, neighbors.at(x - 1, y + 1));

    // Do not use a mirrored diagonal if we already have a diagonal.
    for (auto &block : borderBlock.covers)
    {
        auto current = currentCover.getCover(block.brush);
        if (current && current->featureCover & Diagonals)
        {
            block.featureCover &= ~(Diagonals);
        }
        if (current && current->borderCover & Diagonals)
        {
            block.borderCover &= ~(Diagonals);
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
        cover.featureCover = TileCovers::unifyTileCover(cover.featureCover, TileQuadrant::BottomRight);
        cover.borderCover = TileCovers::unifyTileCover(cover.borderCover, TileQuadrant::BottomRight);

        // Special cases
        if (!Settings::PLACE_MOUNTAIN_FEATURES)
        {
            TileCover &featureCover = cover.featureCover;
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

    // // fixBorderEdgeCases(x, y, cover, neighbors);

    neighbors.set(x, y, borderBlock);

    // borderBlock.sort();

    for (const auto &block : borderBlock.covers)
    {
        auto cover = block.featureCover;
        auto brush = block.brush;

        if (cover & Full)
        {
            applyFeature(mapView, pos, brush, BorderType::NorthWestDiagonal);
            applyFeature(mapView, pos, brush, BorderType::SouthEastDiagonal);
            return;
        }

        // // Sides
        // if (cover & North)
        // {
        //     applyFeature(mapView, pos, brush, BorderType::North);
        // }
        // if (cover & East)
        // {
        //     applyFeature(mapView, pos, brush, BorderType::East);
        // }
        // if (cover & South)
        // {
        //     applyFeature(mapView, pos, brush, BorderType::South);
        // }
        // if (cover & West)
        // {
        //     applyFeature(mapView, pos, brush, BorderType::West);
        // }

        // // Diagonals
        // if (cover & Diagonals)
        // {
        //     if (cover & NorthWest)
        //     {
        //         applyFeature(mapView, pos, brush, BorderType::NorthWestDiagonal);
        //     }
        //     else if (cover & NorthEast)
        //     {
        //         applyFeature(mapView, pos, brush, BorderType::NorthEastDiagonal);
        //     }
        //     else if (cover & SouthEast)
        //     {
        //         applyFeature(mapView, pos, brush, BorderType::SouthEastDiagonal);
        //     }
        //     else if (cover & SouthWest)
        //     {
        //         applyFeature(mapView, pos, brush, BorderType::SouthWestDiagonal);
        //     }
        // }

        // // Corners
        // if (cover & Corners)
        // {
        //     if (cover & NorthEastCorner)
        //     {
        //         applyFeature(mapView, pos, brush, BorderType::NorthEastCorner);
        //     }
        //     if (cover & NorthWestCorner)
        //     {
        //         applyFeature(mapView, pos, brush, BorderType::NorthWestCorner);
        //     }
        //     if (cover & SouthEastCorner)
        //     {
        //         applyFeature(mapView, pos, brush, BorderType::SouthEastCorner);
        //     }
        //     if (cover & SouthWestCorner)
        //     {
        //         applyFeature(mapView, pos, brush, BorderType::SouthWestCorner);
        //     }
        // }

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

MountainPart::MountainCover::MountainCover(TileCover featureCover, TileCover borderCover, MountainBrush *brush)
    : featureCover(featureCover), borderCover(borderCover), brush(brush) {}

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

void MountainNeighborMap::mirrorNorth(value_type &source, const value_type &borders)
{
    if (borders.ground)
    {
        TileCover cover = TileCovers::mirrorNorth(TILE_COVER_FULL);
        source.addFeature(cover, borders.ground);
        source.addBorder(cover, borders.ground);
        return;
    }

    uint32_t sourceZ = source.zOrder();
    for (const auto &cover : borders.covers)
    {
        // uint32_t z = cover.brush->preferredZOrder();
        uint32_t z = 10000;
        if (z > sourceZ)
        {

            source.addFeature(TileCovers::mirrorNorth(cover.featureCover), cover.brush);
            source.addBorder(TileCovers::mirrorNorth(cover.borderCover), cover.brush);
        }
    }

    // addGroundBorder(source, borders, TILE_COVER_SOUTH);
}

void MountainNeighborMap::mirrorEast(value_type &source, const value_type &borders)
{
    if (borders.ground)
    {
        TileCover cover = TileCovers::mirrorEast(TILE_COVER_FULL);
        source.addFeature(cover, borders.ground);
        source.addBorder(cover, borders.ground);
        return;
    }

    uint32_t sourceZ = source.zOrder();
    for (const auto &cover : borders.covers)
    {
        // uint32_t z = cover.brush->preferredZOrder();
        uint32_t z = 10000;
        if (z > sourceZ)
        {

            source.addFeature(TileCovers::mirrorEast(cover.featureCover), cover.brush);
            source.addBorder(TileCovers::mirrorEast(cover.borderCover), cover.brush);
        }
    }

    // addGroundBorder(source, borders, TILE_COVER_SOUTH);
}

void MountainNeighborMap::mirrorSouth(value_type &source, const value_type &borders)
{
    if (borders.ground)
    {
        TileCover cover = TileCovers::mirrorSouth(TILE_COVER_FULL);
        source.addFeature(cover, borders.ground);
        source.addBorder(cover, borders.ground);
        return;
    }

    uint32_t sourceZ = source.zOrder();
    for (const auto &cover : borders.covers)
    {
        // uint32_t z = cover.brush->preferredZOrder();
        uint32_t z = 10000;
        if (z > sourceZ)
        {

            source.addFeature(TileCovers::mirrorSouth(cover.featureCover), cover.brush);
            source.addBorder(TileCovers::mirrorSouth(cover.borderCover), cover.brush);
        }
    }

    // addGroundBorder(source, borders, TILE_COVER_SOUTH);
}

void MountainNeighborMap::mirrorWest(value_type &source, const value_type &borders)
{
    if (borders.ground)
    {
        TileCover cover = TileCovers::mirrorWest(TILE_COVER_FULL);
        source.addFeature(cover, borders.ground);
        source.addBorder(cover, borders.ground);
        return;
    }

    uint32_t sourceZ = source.zOrder();
    for (const auto &cover : borders.covers)
    {
        // uint32_t z = cover.brush->preferredZOrder();
        uint32_t z = 10000;
        if (z > sourceZ)
        {

            source.addFeature(TileCovers::mirrorWest(cover.featureCover), cover.brush);
            source.addBorder(TileCovers::mirrorWest(cover.borderCover), cover.brush);
        }
    }

    // addGroundBorder(source, borders, TILE_COVER_SOUTH);
}

void MountainNeighborMap::addGroundBorder(value_type &self, const value_type &other, TileCover border)
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
            auto brush = other.ground;
            if (brush)
            {
                self.add(border, brush);
            }
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
}

void MountainNeighborMap::mirrorSouthWest(value_type &source, const value_type &borders)
{
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

void MountainBrush::applyBorder(MapView &mapView, const Position &position, MountainBrush *brush, BorderType borderType)
{
    auto borderItemId = brush->outerBorder->value()->getServerId(borderType);

    if (borderItemId && (!isFeature(TileCovers::fromBorderType(borderType)) || Settings::PLACE_MOUNTAIN_FEATURES))
    {
        mapView.addBorder(position, *borderItemId, brush->outerBorder->value()->preferredZOrder());
    }
}