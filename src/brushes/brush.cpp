#include "brush.h"

#include "../items.h"
#include "ground_brush.h"
#include "raw_brush.h"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "../../vendor/fts_fuzzy_match/fts_fuzzy_match.h"

vme_unordered_map<uint32_t, std::unique_ptr<Brush>> Brush::rawBrushes;
vme_unordered_map<std::string, std::unique_ptr<Brush>> Brush::groundBrushes;

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
            "Could not add ground brush '{}' with id '{}'. A brush with id '{}' (named '{}') already exists.",
            brush->name(), brushId, brushId, found->second->name()));
        return nullptr;
    }

    auto result = groundBrushes.emplace(brushId, std::move(brush));

    return static_cast<GroundBrush *>(result.first.value().get());
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

const vme_unordered_map<uint32_t, std::unique_ptr<Brush>> &Brush::getRawBrushes()
{
    return rawBrushes;
}

std::unique_ptr<std::vector<Brush *>> Brush::search(std::string searchString)
{
    using Match = std::pair<int, Brush *>;
    std::vector<Match> matches = searchWithScore(searchString);

    auto results = std::make_unique<std::vector<Brush *>>();
    results->reserve(matches.size());

    std::transform(matches.begin(), matches.end(), std::back_inserter(*results),
                   [](Match match) -> Brush * { return match.second; });

    return results;
}

std::vector<std::pair<int, Brush *>> Brush::searchWithScore(std::string searchString)
{
    using Match = std::pair<int, Brush *>;
    std::vector<Match> matches;

    auto f = [](const Match &lhs, const Match &rhs) {
        return lhs.first > rhs.first;
    };

    for (const auto &[_, rawBrush] : rawBrushes)
    {
        int score;
        bool match = fts::fuzzy_match(searchString.c_str(), rawBrush->name().c_str(), score);

        if (match)
        {
            matches.emplace_back(std::pair{score, rawBrush.get()});
        }
    }

    std::sort(matches.begin(), matches.end(), f);

    return matches;
}