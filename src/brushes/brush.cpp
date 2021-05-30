#include "brush.h"

#include "../items.h"
#include "ground_brush.h"
#include "raw_brush.h"

#define FTS_FUZZY_MATCH_IMPLEMENTATION
#include "../../vendor/fts_fuzzy_match/fts_fuzzy_match.h"

vme_unordered_map<uint32_t, std::unique_ptr<Brush>> Brush::rawBrushes;
vme_unordered_map<uint32_t, std::unique_ptr<Brush>> Brush::groundBrushes;

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
    uint32_t brushId = brush->brushId();

    auto found = groundBrushes.find(brushId);
    if (found != groundBrushes.end())
    {
        VME_LOG_ERROR(
            "Could not add ground brush '" << brush->name() << "' with id '" << brushId
                                           << "'. A brush with name '" << found->second->name()
                                           << "' already uses that brush id. Tip: Change the id to something else.");
        return nullptr;
    }

    auto result = groundBrushes.emplace(brushId, std::move(brush));

    return static_cast<GroundBrush *>(result.first.value().get());
}


std::vector<std::pair<int, Brush *>> Brush::search(std::string searchString)
{
    using Match = std::pair<int, Brush *>;
    std::vector<Match> matches;

    auto f = [](const Match &lhs, const Match &rhs) {
        return lhs.first > rhs.first;
    };

    VME_LOG("Brushes count: " << rawBrushes.size());
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
