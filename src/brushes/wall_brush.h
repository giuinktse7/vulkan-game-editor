#pragma once

#include <string>
#include <unordered_set>

#include "../const.h"
#include "brush.h"

struct Position;
struct WallNeighborMap;

class WallBrush final : public Brush
{
  public:
    struct Door
    {
        Door(uint32_t id, DoorType type, bool open)
            : id(id), type(type), open(open) {}

        uint32_t id;
        DoorType type = DoorType::Normal;
        bool open = false;
    };

    struct Window
    {
        Window(uint32_t id, WindowType type, bool open)
            : id(id), type(type), open(open) {}

        uint32_t id;
        WindowType type = WindowType::Normal;
        bool open = false;
    };

    struct Part
    {
        uint32_t totalWeight = 0;
        std::vector<WeightedItemId> items;
    };

    struct StraightPart : public Part
    {
        std::vector<Door> doors;
        std::vector<Window> windows;
    };

  public:
    WallBrush(std::string id, const std::string &name, StraightPart &&horizontal, StraightPart &&vertical, Part &&corner, Part &&pole);

    void apply(MapView &mapView, const Position &position) override;
    void erase(MapView &mapView, const Position &position) override;

    std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const override;
    std::vector<ThingDrawInfo> getPreviewTextureInfo(Position from, Position to) const;

    void applyInRectangleArea(MapView &mapView, const Position &from, const Position &to);

    const std::string getDisplayId() const override;
    bool erasesItem(uint32_t serverId) const override;
    bool includes(uint32_t serverId) const;

    BrushType type() const override;

    std::string brushId() const noexcept;

    uint32_t iconServerId() const;

    void setIconServerId(uint32_t serverId);

    // Add the brush to its itemtypes
    void finalize();

    const std::vector<uint32_t> serverIds() const;

  private:
    const Part &getPart(WallType type) const;
    uint32_t sampleServerId(WallType type) const;

    void connectWalls(MapView &mapView, WallNeighborMap &neighbors, const Position &center);

    std::unordered_set<uint32_t> _serverIds;

    std::string id;
    uint32_t _iconServerId = 0;

    StraightPart horizontal;
    StraightPart vertical;
    Part corner;
    Part pole;
};

struct WallNeighborMap
{
    WallNeighborMap(const WallBrush *brush, const Position &position, const Map &map);
    bool at(int x, int y) const;

    void set(int x, int y, bool value);

    bool hasWall(int x, int y) const;

  private:
    int index(int x, int y) const;

    std::array<bool, 25> data{};
};