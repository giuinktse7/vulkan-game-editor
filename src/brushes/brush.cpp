#include "brush.h"

#include "../creature.h"
#include "../items.h"
#include "../map.h"
#include "../tile.h"
#include "border_brush.h"
#include "creature_brush.h"
#include "doodad_brush.h"
#include "ground_brush.h"
#include "raw_brush.h"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "../../vendor/fts_fuzzy_match/fts_fuzzy_match.h"

vme_unordered_map<uint32_t, std::unique_ptr<RawBrush>> Brush::rawBrushes;
vme_unordered_map<std::string, std::unique_ptr<GroundBrush>> Brush::groundBrushes;
vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> Brush::borderBrushes;
vme_unordered_map<std::string, std::unique_ptr<DoodadBrush>> Brush::doodadBrushes;
vme_unordered_map<std::string, std::unique_ptr<CreatureBrush>> Brush::creatureBrushes;

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
        Items::items.getItemTypeByServerId(serverId)->brush = groundBrush;
    }

    return groundBrush;
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
        Items::items.getItemTypeByServerId(serverId)->brush = borderBrush;
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

    return static_cast<DoodadBrush *>(result.first.value().get());
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

vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> &Brush::getBorderBrushes()
{
    return borderBrushes;
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