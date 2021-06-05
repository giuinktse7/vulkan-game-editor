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

BrushSearchResult Brush::search(std::string searchString)
{
    using Match = std::pair<int, Brush *>;

    BrushSearchResult searchResult;

    std::vector<Match> matches;

    for (const auto &[_, rawBrush] : rawBrushes)
    {
        int score;
        bool match = fts::fuzzy_match(searchString.c_str(), rawBrush->name().c_str(), score);

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
        return to_underlying(leftBrush->type()) > to_underlying(rightBrush->type());
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
        return to_underlying(leftBrush->type()) > to_underlying(rightBrush->type());
    }

    // Both raw here, no need to check rightBrush (we check if they are the same above)
    if (leftBrush->type() == BrushType::Raw)
    {
        return static_cast<RawBrush *>(leftBrush)->serverId() < static_cast<RawBrush *>(rightBrush)->serverId();
    }

    // Same brushes but not of type Raw, use match score
    return lhs.first < rhs.first;
}
