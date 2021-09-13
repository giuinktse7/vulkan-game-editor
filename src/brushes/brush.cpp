#include "brush.h"

#include "../creature.h"
#include "../items.h"
#include "../map.h"
#include "../tile.h"
#include "border_brush.h"
#include "creature_brush.h"
#include "doodad_brush.h"
#include "ground_brush.h"
#include "mountain_brush.h"
#include "raw_brush.h"
#include "wall_brush.h"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "../../vendor/fts_fuzzy_match/fts_fuzzy_match.h"

vme_unordered_map<uint32_t, std::unique_ptr<RawBrush>> Brush::rawBrushes;
vme_unordered_map<std::string, std::unique_ptr<GroundBrush>> Brush::groundBrushes;
vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> Brush::borderBrushes;
vme_unordered_map<std::string, std::unique_ptr<WallBrush>> Brush::wallBrushes;
vme_unordered_map<std::string, std::unique_ptr<DoodadBrush>> Brush::doodadBrushes;
vme_unordered_map<std::string, std::unique_ptr<CreatureBrush>> Brush::creatureBrushes;
vme_unordered_map<std::string, std::unique_ptr<MountainBrush>> Brush::mountainBrushes;

BrushShape *Brush::_brushShape = new RectangularBrushShape(7, 7);

BrushShape &Brush::brushShape() noexcept
{
    return *_brushShape;
}

void Brush::setBrushShape(BrushShape *brushShape) noexcept
{
    delete _brushShape;
    _brushShape = brushShape;
}

std::unordered_set<Position> CircularBrushShape::Size3x3 = {
    Position(1, 0, 0),

    Position(0, 1, 0),
    Position(1, 1, 0),
    Position(2, 1, 0),

    Position(1, 2, 0),
};

Brush::Brush(std::string name)
    : _name(name) {}

const std::string &Brush::name() const noexcept
{
    return _name;
}

void Brush::setTileset(Tileset *tileset) noexcept
{
    _tileset = tileset;
}

Tileset *Brush::tileset() const noexcept
{
    return _tileset;
}

Brush *Brush::getOrCreateRawBrush(uint32_t serverId)
{
    auto found = rawBrushes.find(serverId);
    if (found == rawBrushes.end())
    {
        rawBrushes.try_emplace(serverId, std::make_unique<RawBrush>(Items::items.getItemTypeByServerId(serverId)));
    }

    return &(*rawBrushes.at(serverId));
}

GroundBrush *Brush::addGroundBrush(GroundBrush &&brush)
{
    return addGroundBrush(std::make_unique<GroundBrush>(std::move(brush)));
}

GroundBrush *Brush::addGroundBrush(std::unique_ptr<GroundBrush> &&brush)
{
    std::string brushId = brush->brushId();

    auto found = groundBrushes.find(brushId);
    if (found != groundBrushes.end())
    {
        VME_LOG_ERROR(std::format(
            "Could not add ground brush '{}' with id '{}'. A ground brush with id '{}' (named '{}') already exists.",
            brush->name(), brushId, brushId, found->second->name()));
        return nullptr;
    }

    auto result = groundBrushes.emplace(brushId, std::move(brush));

    auto groundBrush = static_cast<GroundBrush *>(result.first.value().get());

    // Store brush in the item type
    for (uint32_t serverId : groundBrush->serverIds())
    {
        Items::items.getItemTypeByServerId(serverId)->addBrush(groundBrush);
    }

    return groundBrush;
}

WallBrush *Brush::addWallBrush(WallBrush &&brush)
{
    std::string brushId = brush.brushId();

    auto found = wallBrushes.find(brushId);
    if (found != wallBrushes.end())
    {
        VME_LOG_ERROR(std::format(
            "Could not add wall brush '{}' with id '{}'. A wall brush with id '{}' (named '{}') already exists.",
            brush.name(), brushId, brushId, found->second->name()));
        return nullptr;
    }

    auto result = wallBrushes.emplace(brushId, std::make_unique<WallBrush>(std::move(brush)));

    auto wallBrush = (result.first.value().get());
    wallBrush->finalize();

    return wallBrush;
}

BorderBrush *Brush::addBorderBrush(BorderBrush &&brush)
{
    std::string brushId = brush.brushId();

    auto found = borderBrushes.find(brushId);
    if (found != borderBrushes.end())
    {
        VME_LOG_ERROR(std::format(
            "Could not add border brush '{}' with id '{}'. A border brush with id '{}' (named '{}') already exists.",
            brush.name(), brushId, brushId, found->second->name()));
        return nullptr;
    }

    auto result = borderBrushes.emplace(brushId, std::make_unique<BorderBrush>(std::move(brush)));

    auto borderBrush = static_cast<BorderBrush *>(result.first.value().get());

    // Store brush in the item type
    for (uint32_t serverId : borderBrush->serverIds())
    {
        ItemType *itemType = Items::items.getItemTypeByServerId(serverId);

        itemType->addBrush(borderBrush);
    }

    return borderBrush;
}

GroundBrush *Brush::getGroundBrush(const std::string &id)
{
    auto found = groundBrushes.find(id);
    if (found == groundBrushes.end())
    {
        return nullptr;
    }

    return static_cast<GroundBrush *>(found->second.get());
}

MountainBrush *Brush::addMountainBrush(std::unique_ptr<MountainBrush> &&brush)
{
    std::string brushId = brush->id();

    auto found = mountainBrushes.find(brushId);
    if (found != mountainBrushes.end())
    {
        VME_LOG_ERROR(std::format(
            "Could not add doodad brush '{}' with id '{}'. A doodad brush with id '{}' (named '{}') already exists.",
            brush->name(), brushId, brushId, found->second->name()));
        return nullptr;
    }

    auto result = mountainBrushes.emplace(brushId, std::move(brush));

    MountainBrush *mountainBrush = static_cast<MountainBrush *>(result.first.value().get());
    for (uint32_t id : mountainBrush->serverIds())
    {
        Items::items.getItemTypeByServerId(id)->addBrush(mountainBrush);
    }

    return mountainBrush;
}

MountainBrush *Brush::addMountainBrush(MountainBrush &&brush)
{
    return addMountainBrush(std::make_unique<MountainBrush>(std::move(brush)));
}

MountainBrush *Brush::getMountainBrush(const std::string &id)
{
    auto found = mountainBrushes.find(id);
    if (found == mountainBrushes.end())
    {
        return nullptr;
    }

    return static_cast<MountainBrush *>(found->second.get());
}

DoodadBrush *Brush::addDoodadBrush(DoodadBrush &&brush)
{
    return addDoodadBrush(std::make_unique<DoodadBrush>(std::move(brush)));
}

DoodadBrush *Brush::addDoodadBrush(std::unique_ptr<DoodadBrush> &&brush)
{
    std::string brushId = brush->id();

    auto found = doodadBrushes.find(brushId);
    if (found != doodadBrushes.end())
    {
        VME_LOG_ERROR(std::format(
            "Could not add doodad brush '{}' with id '{}'. A doodad brush with id '{}' (named '{}') already exists.",
            brush->name(), brushId, brushId, found->second->name()));
        return nullptr;
    }

    auto result = doodadBrushes.emplace(brushId, std::move(brush));

    DoodadBrush *doodadBrush = static_cast<DoodadBrush *>(result.first.value().get());
    for (uint32_t id : doodadBrush->serverIds())
    {
        Items::items.getItemTypeByServerId(id)->addBrush(doodadBrush);
    }

    return doodadBrush;
}

DoodadBrush *Brush::getDoodadBrush(const std::string &id)
{
    auto found = doodadBrushes.find(id);
    if (found == doodadBrushes.end())
    {
        return nullptr;
    }

    return static_cast<DoodadBrush *>(found->second.get());
}

CreatureBrush *Brush::addCreatureBrush(CreatureBrush &&brush)
{
    return addCreatureBrush(std::make_unique<CreatureBrush>(std::move(brush)));
}

CreatureBrush *Brush::addCreatureBrush(std::unique_ptr<CreatureBrush> &&brush)
{
    std::string brushId = brush->id();

    auto found = creatureBrushes.find(brushId);
    if (found != creatureBrushes.end())
    {
        VME_LOG_ERROR(std::format(
            "Could not add creature brush '{}' with id '{}'. A creature brush with id '{}' (named '{}') already exists.",
            brush->name(), brushId, brushId, found->second->name()));
        return nullptr;
    }

    auto result = creatureBrushes.emplace(brushId, std::move(brush));

    return static_cast<CreatureBrush *>(result.first.value().get());
}

CreatureBrush *Brush::getCreatureBrush(const std::string &id)
{
    auto found = creatureBrushes.find(id);
    if (found == creatureBrushes.end())
    {
        return nullptr;
    }

    return static_cast<CreatureBrush *>(found->second.get());
}

BorderBrush *Brush::getBorderBrush(const std::string &id)
{
    auto found = borderBrushes.find(id);
    if (found == borderBrushes.end())
    {
        return nullptr;
    }

    return static_cast<BorderBrush *>(found->second.get());
}

const vme_unordered_map<uint32_t, std::unique_ptr<RawBrush>> &Brush::getRawBrushes()
{
    return rawBrushes;
}

BrushSearchResult Brush::search(std::string searchString)
{
    using Match = std::pair<int, Brush *>;

    BrushSearchResult searchResult;

    std::vector<Match> matches;

    for (const auto &[_, rawBrush] : rawBrushes)
    {
        int score = 1;
        bool match = fts::fuzzy_match(searchString.c_str(), rawBrush->name().c_str(), score) ||
                     std::to_string(rawBrush->serverId()).find(searchString) != std::string::npos;

        if (match)
        {
            matches.emplace_back(std::pair{score, rawBrush.get()});
            ++searchResult.rawCount;
        }
    }

    for (const auto &[_, groundBrush] : groundBrushes)
    {
        int score;
        bool match = fts::fuzzy_match(searchString.c_str(), groundBrush->name().c_str(), score);

        if (match)
        {
            matches.emplace_back(std::pair{score, groundBrush.get()});
            ++searchResult.groundCount;
        }
    }

    for (const auto &[_, doodadBrush] : doodadBrushes)
    {
        int score;
        bool match = fts::fuzzy_match(searchString.c_str(), doodadBrush->name().c_str(), score);

        if (match)
        {
            matches.emplace_back(std::pair{score, doodadBrush.get()});
            ++searchResult.doodadCount;
        }
    }

    for (const auto &[_, creatureBrush] : creatureBrushes)
    {
        int score;
        bool match = fts::fuzzy_match(searchString.c_str(), creatureBrush->name().c_str(), score);

        if (match)
        {
            matches.emplace_back(std::pair{score, creatureBrush.get()});
            ++searchResult.creatureCount;
        }
    }

    std::sort(matches.begin(), matches.end(), Brush::matchSorter);

    searchResult.matches = std::make_unique<std::vector<Brush *>>();
    searchResult.matches->reserve(matches.size());

    std::transform(matches.begin(), matches.end(), std::back_inserter(*searchResult.matches),
                   [](Match match) -> Brush * { return match.second; });

    return searchResult;
}

bool Brush::brushSorter(const Brush *leftBrush, const Brush *rightBrush)
{
    if (leftBrush->type() != rightBrush->type())
    {
        return to_underlying(leftBrush->type()) < to_underlying(rightBrush->type());
    }

    // Both raw here, no need to check rightBrush (we check if they are the same above)
    if (leftBrush->type() == BrushType::Raw)
    {
        return static_cast<const RawBrush *>(leftBrush)->serverId() > static_cast<const RawBrush *>(rightBrush)->serverId();
    }

    // Same brushes but not of type Raw, use match score
    return leftBrush->name() < rightBrush->name();
}

bool Brush::matchSorter(std::pair<int, Brush *> &lhs, const std::pair<int, Brush *> &rhs)
{
    auto leftBrush = lhs.second;
    auto rightBrush = rhs.second;
    if (leftBrush->type() != rightBrush->type())
    {
        return to_underlying(leftBrush->type()) < to_underlying(rightBrush->type());
    }

    // Both raw here, no need to check rightBrush (we check if they are the same above)
    if (leftBrush->type() == BrushType::Raw)
    {
        return static_cast<RawBrush *>(leftBrush)->serverId() < static_cast<RawBrush *>(rightBrush)->serverId();
    }

    // Same brushes but not of type Raw, use match score
    return lhs.first < rhs.first;
}

vme_unordered_map<std::string, std::unique_ptr<GroundBrush>> &Brush::getGroundBrushes()
{
    return groundBrushes;
}

vme_unordered_map<std::string, std::unique_ptr<MountainBrush>> &Brush::getMountainBrushes()
{
    return mountainBrushes;
}

vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> &Brush::getBorderBrushes()
{
    return borderBrushes;
}

vme_unordered_map<std::string, std::unique_ptr<WallBrush>> &Brush::getWallBrushes()
{
    return wallBrushes;
}

vme_unordered_map<std::string, std::unique_ptr<DoodadBrush>> &Brush::getDoodadBrushes()
{
    return doodadBrushes;
}

void Brush::updatePreview(int variation)
{
    // Empty
}

void Brush::applyWithoutBorderize(MapView &mapView, const Position &position)
{
    apply(mapView, position);
}

int Brush::variationCount() const
{
    return 1;
}

GroundBrush *Brush::getGroundBrush(const Tile &tile)
{
    if (tile.hasGround())
    {
        Brush *brush = tile.ground()->itemType->getBrush(BrushType::Ground);
        return brush ? static_cast<GroundBrush *>(brush) : nullptr;
    }
    else
    {
        return nullptr;
    }
}

BorderBrush *Brush::getBorderBrush(const Tile &tile)
{
    const Item *prev = nullptr;
    for (const auto &item : tile.items())
    {
        if (!(item->isBorder()))
        {
            break;
        }

        prev = item.get();
    }

    if (prev && prev->isBorder())
    {
        Brush *brush = prev->itemType->getBrush(BrushType::Border);
        if (brush)
        {
            return static_cast<BorderBrush *>(brush);
        }
    }

    if (tile.hasGround())
    {
        Brush *brush = tile.ground()->itemType->getBrush(BrushType::Border);
        return brush ? static_cast<BorderBrush *>(brush) : nullptr;
    }

    return nullptr;
}

DoodadBrush *Brush::getDoodadBrush(const Tile &tile)
{
    for (const auto &item : tile.items())
    {
        Brush *brush = item->itemType->getBrush(BrushType::Doodad);
        if (brush)
        {
            return static_cast<DoodadBrush *>(brush);
        }
    }

    return nullptr;
}

WallBrush *Brush::getWallBrush(const Tile &tile)
{
    for (const auto &item : tile.items())
    {
        Brush *brush = item->itemType->getBrush(BrushType::Wall);
        if (brush)
        {
            return static_cast<WallBrush *>(brush);
        }
    }

    return nullptr;
}

CreatureBrush *Brush::getCreatureBrush(const Tile &tile)
{
    if (tile.hasCreature())
    {
        return Brush::getCreatureBrush(tile.creature()->creatureType.id());
    }
    else
    {
        return nullptr;
    }
}

MountainBrush *Brush::getMountainBrush(const Tile &tile)
{
    for (const auto &item : tile.items())
    {
        Brush *brush = item->itemType->getBrush(BrushType::Mountain);
        if (brush)
        {
            return static_cast<MountainBrush *>(brush);
        }
    }

    return nullptr;
}

std::optional<BrushType> Brush::parseBrushType(std::string s)
{
    switch (string_hash(s.c_str()))
    {
        case "ground"_sh:
            return BrushType::Ground;
        case "raw"_sh:
            return BrushType::Raw;
        case "doodad"_sh:
            return BrushType::Doodad;
        case "creature"_sh:
            return BrushType::Creature;
        case "border"_sh:
            return BrushType::Border;
        case "wall"_sh:
            return BrushType::Wall;
        case "mountain"_sh:
            return BrushType::Mountain;
        default:
            return std::nullopt;
    }
}

DrawItemType::DrawItemType(uint32_t serverId, Position relativePosition)
    : itemType(Items::items.getItemTypeByServerId(serverId)), relativePosition(relativePosition) {}

DrawItemType::DrawItemType(ItemType *itemType, Position relativePosition)
    : itemType(itemType), relativePosition(relativePosition) {}

BorderNeighborMap::BorderNeighborMap(const Position &position, BorderBrush *brush, const Map &map)
{
    for (int dy = -2; dy <= 2; ++dy)
    {
        for (int dx = -2; dx <= 2; ++dx)
        {
            TileCover tileCover = getTileCoverAt(brush, map, position + Position(dx, dy, 0));
            TileQuadrant quadrant = brush->getNeighborQuadrant(dx, dy);
            tileCover = TileCovers::unifyTileCover(tileCover, quadrant);

            auto diagonals = tileCover & TileCovers::Diagonals;
            DEBUG_ASSERT((diagonals == TileCovers::None) || TileCovers::exactlyOneSet(diagonals & TileCovers::Diagonals), "Preferred diagonal must be None or have exactly one diagonal set.");

            set(dx, dy, tileCover);
        }
    }
}

TileCover BorderNeighborMap::getTileCoverAt(BorderBrush *brush, const Map &map, const Position position) const
{
    Tile *tile = map.getTile(position);
    if (!tile)
    {
        return TILE_COVER_NONE;
    }

    return tile->getTileCover(brush);
}

bool BorderNeighborMap::isExpanded(int x, int y) const
{
    auto found = std::find_if(expandedCovers.begin(), expandedCovers.end(), [x, y](const ExpandedTileBlock &block) { return block.x == x && block.y == y; });
    return found != expandedCovers.end();
}

bool BorderNeighborMap::hasExpandedCover() const noexcept
{
    return !expandedCovers.empty();
}

void BorderNeighborMap::addExpandedCover(int x, int y)
{
    expandedCovers.emplace_back(ExpandedTileBlock{x, y});
}

TileCover &BorderNeighborMap::at(int x, int y)
{
    return data[index(x, y)];
}

TileCover &BorderNeighborMap::center()
{
    return data[12];
}

TileCover BorderNeighborMap::at(int x, int y) const
{
    return data[index(x, y)];
}

void BorderNeighborMap::set(int x, int y, TileCover tileCover)
{
    data[index(x, y)] = tileCover;
}

int BorderNeighborMap::index(int x, int y) const
{
    return (y + 2) * 5 + (x + 2);
}

//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>BorderData>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>

void BorderData::setCenterGroundId(const std::string &id)
{
    _centerBrush.emplace(Brush::LazyGround(id));
}

GroundBrush *BorderData::centerBrush() const
{
    return _centerBrush ? _centerBrush->value() : nullptr;
}

BorderType BorderData::getBorderType(uint32_t serverId) const
{
    for (int i = 0; i < borderIds.size(); ++i)
    {
        if (borderIds[i] == serverId)
        {
            return static_cast<BorderType>(i + 1);
        }
    }

    if (extraIds && extraIds->contains(serverId))
    {
        return extraIds->at(serverId);
    }

    if (_centerBrush && _centerBrush->value()->erasesItem(serverId))
    {
        return BorderType::Center;
    }
    else
    {
        return BorderType::None;
    }
}

std::optional<uint32_t> BorderData::getServerId(BorderType borderType) const noexcept
{
    // -1 because index zero in BorderType is BorderType::None
    uint32_t id = borderIds[to_underlying(borderType) - 1];
    return id == 0 ? std::nullopt : std::make_optional(id);
}

std::array<uint32_t, 12> BorderData::getBorderIds() const
{
    return borderIds;
}

bool BorderData::is(uint32_t serverId, BorderType borderType) const
{
    if (borderType == BorderType::Center)
    {
        return _centerBrush && _centerBrush->value()->erasesItem(serverId);
    }

    auto borderItemId = getServerId(borderType);
    return borderItemId && (*borderItemId == serverId);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>Brush Shapes>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>

RectangularBrushShape::RectangularBrushShape(uint16_t width, uint16_t height)
    : width(width), height(height) {}

std::unordered_set<Position> RectangularBrushShape::getRelativePositions() const noexcept
{
    int minX = -width / 2;
    int maxX = width / 2;

    int minY = -height / 2;
    int maxY = height / 2;

    std::unordered_set<Position> positions;
    for (int y = minY; y < maxY; ++y)
    {
        for (int x = minX; x < maxX; ++x)
        {
            positions.emplace(Position(x, y, 0));
        }
    }

    return positions;
}

CircularBrushShape::CircularBrushShape(uint16_t radius)
    : radius(radius) {}

std::unordered_set<Position> CircularBrushShape::getRelativePositions() const noexcept
{
    // TODO implement other sizes
    return Size3x3;
}

Brush::LazyGround::LazyGround(GroundBrush *brush)
    : LazyObject(brush) {}

Brush::LazyGround::LazyGround(std::string groundBrushId)
    : LazyObject([groundBrushId]() {
          GroundBrush *brush = Brush::getGroundBrush(groundBrushId);
          if (!brush)
          {
              ABORT_PROGRAM(std::format("Attempted to retrieve a GroundBrush with id '{}' from a Brush::LazyGround, but the brush did not exist.", groundBrushId));
          }

          return brush;
      }) {}

BorderData::BorderData(std::array<uint32_t, 12> borderIds)
    : borderIds(borderIds) {}

BorderData::BorderData(std::array<uint32_t, 12> borderIds, GroundBrush *centerBrush)
    : borderIds(borderIds), _centerBrush(std::make_optional<Brush::LazyGround>(centerBrush)) {}

void BorderData::setExtraBorderIds(vme_unordered_map<uint32_t, BorderType> &&extraIds)
{
    this->extraIds = std::make_unique<vme_unordered_map<uint32_t, BorderType>>(std::move(extraIds));
}

const vme_unordered_map<uint32_t, BorderType> *BorderData::getExtraBorderIds() const
{
    return extraIds ? extraIds.get() : nullptr;
}
