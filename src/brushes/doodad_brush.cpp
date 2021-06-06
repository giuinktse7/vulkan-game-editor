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

DoodadBrush::DoodadBrush(uint32_t id, const std::string &name, DoodadAlternative &&alternative, uint32_t iconServerId)
    : Brush(name), id(id), _iconServerId(iconServerId)
{
    alternatives.emplace_back(std::move(alternative));
    initialize();
}

DoodadBrush::DoodadBrush(uint32_t id, const std::string &name, std::vector<DoodadAlternative> &&alternatives, uint32_t iconServerId)
    : Brush(name), alternatives(std::move(alternatives)), id(id), _iconServerId(iconServerId)
{
    initialize();
}

void DoodadBrush::apply(MapView &mapView, const Position &position)
{
    auto group = sampleGroup();

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
    return serverIds.find(serverId) != serverIds.end();
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
                    serverIds.emplace(static_cast<DoodadSingle *>(choice.get())->serverId);
                }
                case EntryType::Composite:
                {
                    for (const auto &tile : static_cast<DoodadComposite *>(choice.get())->tiles)
                    {
                        std::copy(tile.serverIds.begin(), tile.serverIds.end(), std::inserter(serverIds, serverIds.end()));
                    }
                }
            }
        }
    }

    _nextGroup = alternatives.at(alternateIndex).sample(_name);

    _brushResource.id = _iconServerId;
    _brushResource.type = BrushResourceType::ItemType;
    _brushResource.variant = 0;
}

std::vector<ItemPreviewInfo> DoodadBrush::sampleGroup()
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

                for (const auto serverId : tile.serverIds)
                {
                    result.emplace_back(serverId, relativePos);
                }
            }

            break;
        }
    }

    return result;
}

uint32_t DoodadBrush::brushId() const noexcept
{
    return id;
}

std::vector<ThingDrawInfo> DoodadBrush::getPreviewTextureInfo() const
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