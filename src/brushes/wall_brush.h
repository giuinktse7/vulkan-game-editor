#pragma once

#include <string>
#include <unordered_set>

#include "../const.h"
#include "brush.h"

struct Position;

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
        std::vector<WeightedItemId> items;
    };

    struct StraightPart : public Part
    {
        std::vector<Door> doors;
        std::vector<Window> windows;
    };

  public:
    WallBrush(std::string id, const std::string &name, StraightPart &&horizontal, StraightPart &&vertical, Part &&corner, Part &&pole);

    void apply(MapView &mapView, const Position &position, Direction direction) override;
    void erase(MapView &mapView, const Position &position, Direction direction) override;

    std::vector<ThingDrawInfo> getPreviewTextureInfo(Direction direction) const override;
    const std::string getDisplayId() const override;
    bool erasesItem(uint32_t serverId) const override;
    BrushType type() const override;

    std::string brushId() const noexcept;

    uint32_t iconServerId() const;

    void setIconServerId(uint32_t serverId);

    // Add the brush to its itemtypes
    void finalize();

    const std::vector<uint32_t> serverIds() const;

  private:
    std::unordered_set<uint32_t> _serverIds;

    std::string id;
    uint32_t _iconServerId = 0;

    StraightPart horizontal;
    StraightPart vertical;
    Part corner;
    Part pole;
};
