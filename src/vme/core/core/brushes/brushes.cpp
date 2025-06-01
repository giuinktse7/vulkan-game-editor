#include "brushes.h"
#include "../items.h"
#include "../map.h"

#include <utility>

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "../vendor/fts_fuzzy_match/fts_fuzzy_match.h"

vme_unordered_map<uint32_t, std::unique_ptr<RawBrush>> Brushes::rawBrushes;
vme_unordered_map<std::string, std::unique_ptr<GroundBrush>> Brushes::groundBrushes;
vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> Brushes::borderBrushes;
vme_unordered_map<std::string, std::unique_ptr<WallBrush>> Brushes::wallBrushes;
vme_unordered_map<std::string, std::unique_ptr<DoodadBrush>> Brushes::doodadBrushes;
vme_unordered_map<std::string, std::unique_ptr<CreatureBrush>> Brushes::creatureBrushes;
vme_unordered_map<std::string, std::unique_ptr<MountainBrush>> Brushes::mountainBrushes;

const int DEFAULT_RECTANGULAR_BRUSH_SIZE = 7;

BrushShape *Brushes::_brushShape = new RectangularBrushShape(DEFAULT_RECTANGULAR_BRUSH_SIZE, DEFAULT_RECTANGULAR_BRUSH_SIZE);

BrushShape &Brushes::brushShape() noexcept
{
    return *_brushShape;
}

RawBrush *Brushes::getOrCreateRawBrush(uint32_t serverId)
{
    auto found = rawBrushes.find(serverId);
    if (found == rawBrushes.end())
    {
        rawBrushes.try_emplace(serverId, std::make_unique<RawBrush>(Items::items.getItemTypeByServerId(serverId)));
    }

    return &(*rawBrushes.at(serverId));
}

GroundBrush *Brushes::addGroundBrush(GroundBrush &&brush)
{
    return addGroundBrush(std::make_unique<GroundBrush>(std::move(brush)));
}

GroundBrush *Brushes::addGroundBrush(std::unique_ptr<GroundBrush> &&brush)
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

    auto *groundBrush = static_cast<GroundBrush *>(result.first.value().get());

    // Store brush in the item type
    for (uint32_t serverId : groundBrush->serverIds())
    {
        Items::items.getItemTypeByServerId(serverId)->addBrush(groundBrush);
    }

    return groundBrush;
}

WallBrush *Brushes::addWallBrush(WallBrush &&brush)
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

    auto *wallBrush = (result.first.value().get());
    wallBrush->finalize();

    return wallBrush;
}

BorderBrush *Brushes::addBorderBrush(BorderBrush &&brush)
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

    auto *borderBrush = static_cast<BorderBrush *>(result.first.value().get());

    // Store brush in the item type
    for (uint32_t serverId : borderBrush->serverIds())
    {
        ItemType *itemType = Items::items.getItemTypeByServerId(serverId);

        itemType->addBrush(borderBrush);
    }

    return borderBrush;
}

GroundBrush *Brushes::getGroundBrush(const std::string &id)
{
    auto found = groundBrushes.find(id);
    if (found == groundBrushes.end())
    {
        return nullptr;
    }

    return static_cast<GroundBrush *>(found->second.get());
}

MountainBrush *Brushes::addMountainBrush(std::unique_ptr<MountainBrush> &&brush)
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

MountainBrush *Brushes::addMountainBrush(MountainBrush &&brush)
{
    return addMountainBrush(std::make_unique<MountainBrush>(std::move(brush)));
}

MountainBrush *Brushes::getMountainBrush(const std::string &id)
{
    auto found = mountainBrushes.find(id);
    if (found == mountainBrushes.end())
    {
        return nullptr;
    }

    return static_cast<MountainBrush *>(found->second.get());
}

DoodadBrush *Brushes::addDoodadBrush(DoodadBrush &&brush)
{
    return addDoodadBrush(std::make_unique<DoodadBrush>(std::move(brush)));
}

DoodadBrush *Brushes::addDoodadBrush(std::unique_ptr<DoodadBrush> &&brush)
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

DoodadBrush *Brushes::getDoodadBrush(const std::string &id)
{
    auto found = doodadBrushes.find(id);
    if (found == doodadBrushes.end())
    {
        return nullptr;
    }

    return static_cast<DoodadBrush *>(found->second.get());
}

CreatureBrush *Brushes::addCreatureBrush(CreatureBrush &&brush)
{
    return addCreatureBrush(std::make_unique<CreatureBrush>(std::move(brush)));
}

CreatureBrush *Brushes::addCreatureBrush(std::unique_ptr<CreatureBrush> &&brush)
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

CreatureBrush *Brushes::getCreatureBrush(const std::string &id)
{
    auto found = creatureBrushes.find(id);
    if (found == creatureBrushes.end())
    {
        return nullptr;
    }

    return static_cast<CreatureBrush *>(found->second.get());
}

BorderBrush *Brushes::getBorderBrush(const std::string &id)
{
    auto found = borderBrushes.find(id);
    if (found == borderBrushes.end())
    {
        return nullptr;
    }

    return static_cast<BorderBrush *>(found->second.get());
}

const vme_unordered_map<uint32_t, std::unique_ptr<RawBrush>> &Brushes::getRawBrushes()
{
    return rawBrushes;
}

BrushSearchResult Brushes::search(const std::string &searchString)
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

    std::sort(matches.begin(), matches.end(), Brushes::matchSorter);

    searchResult.matches = std::make_unique<std::vector<Brush *>>();
    searchResult.matches->reserve(matches.size());

    std::transform(matches.begin(), matches.end(), std::back_inserter(*searchResult.matches),
                   [](Match match) -> Brush * { return match.second; });

    return searchResult;
}

bool Brushes::brushSorter(const Brush *leftBrush, const Brush *rightBrush)
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

bool Brushes::matchSorter(std::pair<int, Brush *> &lhs, const std::pair<int, Brush *> &rhs)
{
    auto *leftBrush = lhs.second;
    auto *rightBrush = rhs.second;
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

vme_unordered_map<std::string, std::unique_ptr<GroundBrush>> &Brushes::getGroundBrushes()
{
    return groundBrushes;
}

vme_unordered_map<std::string, std::unique_ptr<MountainBrush>> &Brushes::getMountainBrushes()
{
    return mountainBrushes;
}

vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> &Brushes::getBorderBrushes()
{
    return borderBrushes;
}

vme_unordered_map<std::string, std::unique_ptr<WallBrush>> &Brushes::getWallBrushes()
{
    return wallBrushes;
}

vme_unordered_map<std::string, std::unique_ptr<DoodadBrush>> &Brushes::getDoodadBrushes()
{
    return doodadBrushes;
}


RawBrush *Brushes::getRawBrush(const Tile &tile)
{
    auto *topItem = tile.getTopItem();
    if (!topItem)
    {
        return nullptr;
    }

    uint32_t id = topItem->serverId();
    return Brushes::getOrCreateRawBrush(id);
}

GroundBrush *Brushes::getGroundBrush(const Tile &tile)
{
    if (tile.hasGround())
    {
        Brush *brush = tile.ground()->itemType->getBrush(BrushType::Ground);
        return brush ? static_cast<GroundBrush *>(brush) : nullptr;
    }

    return nullptr;
}

BorderBrush *Brushes::getBorderBrush(const Tile &tile)
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

DoodadBrush *Brushes::getDoodadBrush(const Tile &tile)
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

WallBrush *Brushes::getWallBrush(const Tile &tile)
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

CreatureBrush *Brushes::getCreatureBrush(const Tile &tile)
{
    if (tile.hasCreature())
    {
        return Brushes::getCreatureBrush(tile.creature()->creatureType.id());
    }

    return nullptr;
}

MountainBrush *Brushes::getMountainBrush(const Tile &tile)
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

std::optional<BrushType> Brushes::parseBrushType(const std::string &brushType)
{
    switch (string_hash(brushType.c_str()))
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
