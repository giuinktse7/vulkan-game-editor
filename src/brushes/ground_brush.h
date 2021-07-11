#pragma once

#include <string>
#include <unordered_set>
#include <variant>

#include "../random.h"
#include "brush.h"

struct Position;
class MapView;
class Tile;

/**
 * Ground Brush
 * 
 * Possible optimization:
 * nextServerId() performs linear search. It is the fastest approach for small lists (< 30 items at least).
 * However, if in the future it is common to use larger ground brushes (40-60+ items) then a possible optimization to try
 * is Walker's Alias Method:
 * https://gamedev.stackexchange.com/a/162996
 * C++ Implementation: https://gist.github.com/Liam0205/0b5786e9bfc73e75eb8180b5400cd1f8
 */

class GroundBrush final : public Brush
{
  public:
    GroundBrush(const std::string &name, std::vector<WeightedItemId> &&weightedIds);
    GroundBrush(std::string id, const std::string &name, std::vector<WeightedItemId> &&weightedIds);
    GroundBrush(std::string id, const std::string &name, std::vector<WeightedItemId> &&weightedIds, uint32_t iconServerId);

    void apply(MapView &mapView, const Position &position, Direction direction) override;
    uint32_t iconServerId() const;

    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;

    void setIconServerId(uint32_t serverId);
    void setName(std::string name);

    uint32_t nextServerId() const;

    std::string brushId() const noexcept;

    std::vector<ThingDrawInfo> getPreviewTextureInfo(Direction direction) const override;

    const std::string getDisplayId() const override;

    const std::unordered_set<uint32_t> &serverIds() const;

    static void restrictReplacement(const Tile *tile);
    static void disableReplacement();
    static void resetReplacementFilter();

    static bool mayPlaceOnTile(Tile &tile);

  private:
    void initialize();
    uint32_t sampleServerId() const;

    std::unordered_set<uint32_t> _serverIds;
    std::vector<WeightedItemId> _weightedIds;

    std::string id;
    uint32_t _iconServerId;

    uint32_t totalWeight = 0;

    /*
      std::monostate   -> May replace any ground
      uint32_t         -> May only replace that ground ID
      GroundBrush*     -> May replace any ground tile that is part of the brush
    */
    static std::variant<std::monostate, uint32_t, const GroundBrush *> replacementFilter;
};