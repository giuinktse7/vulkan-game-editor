#include "doodad_brush.h"

#include <cctype>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "../debug.h"
#include "../items.h"
#include "../map_view.h"
#include "../random.h"
#include "../tile.h"

using DoodadAlternative = DoodadBrush::DoodadAlternative;
using DoodadSingle = DoodadBrush::DoodadSingle;
using DoodadComposite = DoodadBrush::DoodadComposite;

namespace
{
    constexpr float DefaultThickness = 0.25;
    constexpr int MaxPreviewRetries = 5;
} // namespace

DoodadBrush::DoodadBrush(std::string id, const std::string &name, DoodadAlternative &&alternative, uint32_t iconServerId)
    : Brush(name), _id(id), _iconServerId(iconServerId), thickness(DefaultThickness)
{
    alternatives.emplace_back(std::move(alternative));
    initialize();
}

DoodadBrush::DoodadBrush(std::string id, const std::string &name, std::vector<DoodadAlternative> &&alternatives, uint32_t iconServerId)
    : Brush(name), alternatives(std::move(alternatives)), _id(id), _iconServerId(iconServerId), thickness(DefaultThickness)
{
    initialize();
}

void DoodadBrush::erase(MapView &mapView, const Position &position)
{
    for (const auto &relativePos : Brush::brushShape().getRelativePositions())
    {
        auto finalPos = position + relativePos;

        if (!mapView.isValidPos(finalPos))
            continue;

        Tile &tile = mapView.getOrCreateTile(finalPos);

        const Item *item = tile.getItem([this](const Item &item) {
            return this->erasesItem(item.serverId());
        });

        if (!item)
        {
            continue;
        }

        uint32_t serverId = item->serverId();

        // If the item belongs to a composite, then remove the whole composite
        auto found = composites.find(serverId);
        if (found != composites.end())
        {
            auto composite = found->second;
            Position zeroPos = finalPos - composite->relativePosition(serverId);
            for (const auto &tile : composite->tiles)
            {
                mapView.removeItems(zeroPos + tile.relativePosition(), [tile](const Item &item) {
                    return item.serverId() == tile.serverId;
                });
            }
        }
        else
        {
            mapView.removeItems(tile, [this](const Item &item) { return this->erasesItem(item.serverId()); });
        }
    }
}

Position DoodadBrush::CompositeTile::relativePosition() const
{
    return Position(dx, dy, dz);
}

Position DoodadComposite::relativePosition(uint32_t serverId)
{
    for (const auto &tile : tiles)
    {
        if (tile.serverId == serverId)
        {
            return tile.relativePosition();
        }
    }

    ABORT_PROGRAM(std::format("The DoodadComposite did not contain the serverId", serverId));
}

bool DoodadBrush::prepareApply(MapView &mapView, const Position &position)
{
    switch (replaceBehavior)
    {
        case ReplaceBehavior::Block:
        {
            auto *tile = mapView.getTile(position);
            if (tile)
            {
                bool blocked = tile->containsItem([this](const Item &item) {
                    return item.itemType->brush() == this;
                });

                if (blocked)
                {
                    return false;
                }
            }
        }
        case ReplaceBehavior::Replace:
        {
            auto *tile = mapView.getTile(position);
            if (tile)
            {
                tile->removeItemsIf([this](const Item &item) {
                    return item.itemType->brush() == this;
                });
            }
        }
        break;
        default:
            break;
    }

    return true;
}

void DoodadBrush::apply(MapView &mapView, const Position &position)
{
    // Take a sample even if it is blocked
    int alternateIndex = util::modulo(mapView.getBrushVariation(), alternatives.size());
    auto group = _buffer;

    for (const auto &entry : _buffer)
    {
        const Position finalPos = position + entry.relativePosition;

        if (!mapView.isValidPos(finalPos))
            continue;

        uint32_t serverId = entry.serverId;

        bool placeItem = prepareApply(mapView, finalPos);

        if (placeItem)
        {
            mapView.addItem(finalPos, Item(serverId), false);
        }
    }

    updatePreview(alternateIndex);
}

uint32_t DoodadBrush::iconServerId() const
{
    return _iconServerId;
}

bool DoodadBrush::erasesItem(uint32_t serverId) const
{
    return _serverIds.find(serverId) != _serverIds.end();
}

BrushType DoodadBrush::type() const
{
    return BrushType::Doodad;
}

void DoodadBrush::initialize()
{
    for (const auto &alternative : alternatives)
    {
        for (const auto &choice : alternative.choices)
        {
            switch (choice->type)
            {
                case EntryType::Single:
                {
                    _serverIds.emplace(static_cast<DoodadSingle *>(choice.get())->serverId);
                    break;
                }
                case EntryType::Composite:
                {

                    auto composite = static_cast<DoodadComposite *>(choice.get());
                    for (const auto &tile : composite->tiles)
                    {
                        composites.emplace(tile.serverId, composite);
                        _serverIds.emplace(tile.serverId);
                    }
                    break;
                }
            }
        }
    }

    _buffer = alternatives.at(0).sample(_name);
}

std::unordered_set<uint32_t> DoodadBrush::serverIds() const
{
    return _serverIds;
}

DoodadBrush::DoodadSingle::DoodadSingle(uint32_t serverId, uint32_t weight)
    : DoodadEntry(weight, EntryType::Single), serverId(serverId) {}

DoodadBrush::DoodadComposite::DoodadComposite(std::vector<CompositeTile> &&tiles, uint32_t weight)
    : DoodadEntry(weight, EntryType::Composite), tiles(std::move(tiles)) {}

DoodadAlternative::DoodadAlternative(std::vector<std::unique_ptr<DoodadEntry>> &&choices)
    : choices(std::move(choices))
{

    // Sort by weights descending to optimize iteration in sample() for the most common cases.
    std::sort(
        this->choices.begin(),
        this->choices.end(), [](const std::unique_ptr<DoodadEntry> &a, const std::unique_ptr<DoodadEntry> &b) {
            return a->weight > b->weight;
        });

    for (const auto &choice : this->choices)
    {
        totalWeight += choice->weight;
        choice->weight = totalWeight;
    }
}

std::vector<ItemPreviewInfo> DoodadAlternative::sample(const std::string &brushName) const
{
    std::vector<ItemPreviewInfo> result{};
    if (choices.empty())
        return result;

    uint32_t weight = Random::global().nextInt<uint32_t>(static_cast<uint32_t>(0), totalWeight);

    DoodadEntry *found;

    for (const auto &entry : choices)
    {
        if (weight < entry->weight)
        {
            found = entry.get();
            break;
        }
    }

    if (found == nullptr)
    {
        // If we get here, something is off with `weight` for some entry or with `totalWeight`.
        VME_LOG_ERROR(
            "[GroundBrush::nextServerId] Brush " << brushName
                                                 << ": Could not find matching weight for randomly generated weight "
                                                 << weight << " (totalWeight: " << totalWeight << ".");

        // No match in for-loop. Use the first entry.
        found = choices.at(0).get();
    }

    switch (found->type)
    {
        case EntryType::Single:
        {
            auto single = static_cast<DoodadSingle *>(found);
            result.emplace_back(single->serverId, PositionConstants::Zero);
            break;
        }
        case EntryType::Composite:
        {
            auto composite = static_cast<DoodadComposite *>(found);
            for (const CompositeTile &tile : composite->tiles)
            {
                const Position relativePos(
                    static_cast<Position::value_type>(tile.dx),
                    static_cast<Position::value_type>(tile.dy),
                    static_cast<Position::value_type>(tile.dz));

                result.emplace_back(tile.serverId, relativePos);
            }

            break;
        }
    }

    return result;
}

const std::string &DoodadBrush::id() const noexcept
{
    return _id;
}

int DoodadBrush::variationCount() const
{
    return alternatives.size();
}

void DoodadBrush::updatePreview(int variation)
{
    _buffer.clear();
    auto relativePositions = Brush::brushShape().getRelativePositions();
    std::unordered_set<Position> takenPositions;

    int alternateIndex = util::modulo(variation, alternatives.size());

    while (!relativePositions.empty())
    {
        auto it = relativePositions.begin();
        Position relativePos = *it;
        relativePositions.erase(it);

        // Skip positions based on thickness
        if (Random::global().nextDouble() > thickness)
        {
            continue;
        }

        int retries = 0;
        while (retries < MaxPreviewRetries)
        {
            bool ok = true;
            auto sample = alternatives.at(alternateIndex).sample(_name);

            for (const auto &previewInfo : sample)
            {
                if (takenPositions.contains(relativePos + previewInfo.relativePosition))
                {
                    // Can not place this doodad. Try again.
                    ++retries;
                    ok = false;
                    break;
                }
            }

            if (ok)
            {
                for (const auto &previewInfo : sample)
                {
                    auto finalPos = relativePos + previewInfo.relativePosition;
                    takenPositions.emplace(finalPos);
                    relativePositions.erase(previewInfo.relativePosition);
                    _buffer.emplace_back(ItemPreviewInfo(previewInfo.serverId, finalPos));
                }

                break;
            }
        }
    }

    this->alternateIndex = alternateIndex;
}

std::vector<ThingDrawInfo> DoodadBrush::getPreviewTextureInfo(int variation) const
{
    std::vector<ThingDrawInfo> previewInfo;

    std::ranges::transform(_buffer, std::back_inserter(previewInfo),
                           [](const ItemPreviewInfo &info) -> ThingDrawInfo {
                               return DrawItemType(info.serverId, info.relativePosition);
                           });

    return previewInfo;
}

const std::string DoodadBrush::getDisplayId() const
{
    return _name;
}

DoodadBrush::ReplaceBehavior DoodadBrush::parseReplaceBehavior(std::string raw)
{
    switch (string_hash(raw.c_str()))
    {
        case "block"_sh:
            return ReplaceBehavior::Block;
        case "replace"_sh:
            return ReplaceBehavior::Replace;
        case "stack"_sh:
            return ReplaceBehavior::Stack;
        default:
            VME_LOG(std::format("Unknown DoodadBrush replace behavior: {}", raw));
            return ReplaceBehavior::Block;
    }
}
