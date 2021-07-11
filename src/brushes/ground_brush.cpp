
#include "ground_brush.h"

#include <numeric>

#include "../debug.h"
#include "../items.h"
#include "../map_view.h"
#include "../position.h"
#include "../random.h"
#include "../tile.h"

std::variant<std::monostate, uint32_t, const GroundBrush *> GroundBrush::replacementFilter = std::monostate{};

GroundBrush::GroundBrush(std::string id, const std::string &name, std::vector<WeightedItemId> &&weightedIds)
    : Brush(name), _weightedIds(std::move(weightedIds)), id(id), _iconServerId(_weightedIds.at(0).id)
{
    initialize();
}

GroundBrush::GroundBrush(std::string id, const std::string &name, std::vector<WeightedItemId> &&weightedIds, uint32_t iconServerId)
    : Brush(name), _weightedIds(std::move(weightedIds)), id(id), _iconServerId(iconServerId)
{
    initialize();
}

GroundBrush::GroundBrush(const std::string &id, std::vector<WeightedItemId> &&weightedIds)
    : Brush(id), _weightedIds(std::move(weightedIds)), id(id), _iconServerId(_weightedIds.at(0).id)
{
    initialize();
}

void GroundBrush::setIconServerId(uint32_t serverId)
{
    _iconServerId = serverId;
}

void GroundBrush::initialize()
{
    // Sort by weights descending to optimize iteration in sampleServerId() for the most common cases.
    std::sort(_weightedIds.begin(), _weightedIds.end(), [](const WeightedItemId &a, const WeightedItemId &b) { return a.weight > b.weight; });

    for (auto &entry : _weightedIds)
    {
        totalWeight += entry.weight;
        entry.weight = totalWeight;
        _serverIds.emplace(entry.id);
    }
}

void GroundBrush::apply(MapView &mapView, const Position &position, Direction direction)
{
    // TODO Implement the apply correctly for AutoBorder

    Tile &tile = mapView.getOrCreateTile(position);
    if (mayPlaceOnTile(tile))
    {
        mapView.addItem(position, Item(nextServerId()));
    }
}

bool GroundBrush::mayPlaceOnTile(Tile &tile)
{
    if (replacementFilter.index() == 0)
    {
        return true;
    }

    bool result = false;
    std::visit(
        util::overloaded{
            [&tile, &result](uint32_t serverId) {
                Item *ground = tile.ground();
                if (ground && ground->serverId() == serverId)
                {
                    result = true;
                }
            },
            [&tile, &result](const GroundBrush *groundBrush) {
                if (groundBrush)
                {
                    if (tile.hasGround() && groundBrush == tile.ground()->itemType->brush)
                    {
                        result = true;
                    }
                }
                else
                {
                    // Only tiles without ground
                    if (!tile.hasGround())
                    {
                        result = true;
                    }
                }
            },
            [](const auto &arg) {}},
        replacementFilter);

    return result;
}

uint32_t GroundBrush::iconServerId() const
{
    return _iconServerId;
}

BrushType GroundBrush::type() const
{
    return BrushType::Ground;
}

bool GroundBrush::erasesItem(uint32_t serverId) const
{
    return _serverIds.find(serverId) != _serverIds.end();
}

uint32_t GroundBrush::nextServerId() const
{
    return sampleServerId();
}

uint32_t GroundBrush::sampleServerId() const
{
    uint32_t weight = Random::global().nextInt<uint32_t>(static_cast<uint32_t>(0), totalWeight);

    for (const auto &entry : _weightedIds)
    {
        if (weight < entry.weight)
        {
            return entry.id;
        }
    }

    // If we get here, something is off with `weight` for some weighted ID or with `totalWeight`.
    VME_LOG_ERROR(
        "[GroundBrush::nextServerId] Brush " << _name
                                             << ": Could not find matching weight for randomly generated weight "
                                             << weight << " (totalWeight: " << totalWeight << ".");

    // No match in for-loop. Use the first ID.
    return _weightedIds.at(0).id;
}

std::string GroundBrush::brushId() const noexcept
{
    return id;
}

void GroundBrush::setName(std::string name)
{
    _name = name;
}

std::vector<ThingDrawInfo> GroundBrush::getPreviewTextureInfo(Direction direction) const
{
    return std::vector<ThingDrawInfo>{DrawItemType(_iconServerId, PositionConstants::Zero)};
}

const std::string GroundBrush::getDisplayId() const
{
    return id;
}

const std::unordered_set<uint32_t> &GroundBrush::serverIds() const
{
    return _serverIds;
}

void GroundBrush::restrictReplacement(const Tile *tile)
{
    if (!tile || !tile->ground())
    {
        replacementFilter = static_cast<const GroundBrush *>(nullptr);
        return;
    }

    Item *ground = tile->ground();
    auto tileGroundBrush = ground->itemType->brush;
    if (tileGroundBrush && tileGroundBrush->type() == BrushType::Ground)
    {
        replacementFilter = static_cast<GroundBrush *>(tileGroundBrush);
    }
    else
    {
        replacementFilter = ground->serverId();
    }
}

void GroundBrush::disableReplacement()
{
    replacementFilter = static_cast<const GroundBrush *>(nullptr);
}

void GroundBrush::resetReplacementFilter()
{
    replacementFilter = std::monostate{};
}
