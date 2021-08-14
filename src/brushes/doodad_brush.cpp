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
}

DoodadBrush::DoodadBrush(std::string id, const std::string &name, DoodadAlternative &&alternative, uint32_t iconServerId)
    : Brush(name), _id(id), _iconServerId(iconServerId)
{
    alternatives.emplace_back(std::move(alternative));
    initialize();
}

DoodadBrush::DoodadBrush(std::string id, const std::string &name, std::vector<DoodadAlternative> &&alternatives, uint32_t iconServerId)
    : Brush(name), alternatives(std::move(alternatives)), _id(id), _iconServerId(iconServerId)
{
    initialize();
}

void DoodadBrush::erase(MapView &mapView, const Position &position)
{
    Tile &tile = mapView.getOrCreateTile(position);

    const Item *item = tile.getItem([this](const Item &item) {
        return this->erasesItem(item.serverId());
    });

    if (!item)
    {
        return;
    }

    uint32_t serverId = item->serverId();

    // If the item belongs to a composite, then remove the whole composite
    auto found = composites.find(serverId);
    if (found != composites.end())
    {
        auto composite = found->second;
        Position zeroPos = position - composite->relativePosition(serverId);
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

void DoodadBrush::apply(MapView &mapView, const Position &position)
{
    // Take a sample even if it is blocked
    int alternateIndex = util::modulo(mapView.getBrushVariation(), alternatives.size());
    auto group = sampleGroup(alternateIndex);

    switch (replaceBehavior)
    {
        case ReplaceBehavior::Block:
        {
            auto *tile = mapView.getTile(position);
            if (tile)
            {
                bool blocked = tile->containsItem([this](const Item &item) {
                    return item.itemType->brush == this;
                });

                if (blocked)
                {
                    return;
                }
            }
        }
        case ReplaceBehavior::Replace:
        {
            auto *tile = mapView.getTile(position);
            if (tile)
            {
                tile->removeItemsIf([this](const Item &item) {
                    return item.itemType->brush == this;
                });
            }
        }
        break;
        default:
            break;
    }

    for (const auto &entry : group)
    {
        mapView.addItem(position + entry.relativePosition, Item(entry.serverId));
    }
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

    _nextGroup = alternatives.at(0).sample(_name);
}

std::unordered_set<uint32_t> DoodadBrush::serverIds() const
{
    return _serverIds;
}

std::vector<ItemPreviewInfo> DoodadBrush::sampleGroup(int alternateIndex)
{
    std::vector<ItemPreviewInfo> result = _nextGroup;
    _nextGroup = alternatives.at(alternateIndex).sample(_name);

    return result;
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
    int alternateIndex = util::modulo(variation, alternatives.size());
    _nextGroup = alternatives.at(alternateIndex).sample(_name);
}

std::vector<ThingDrawInfo> DoodadBrush::getPreviewTextureInfo(int variation) const
{
    std::vector<ThingDrawInfo> previewInfo;

    std::ranges::transform(_nextGroup, std::back_inserter(previewInfo),
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
