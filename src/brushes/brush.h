#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../position.h"
#include "../util.h"

struct Position;
class Tileset;
class MapView;
class GroundBrush;

enum class BrushType
{
    Raw,
    Ground,
    Doodad
};

struct WeightedItemId
{
    WeightedItemId(uint32_t id, uint32_t weight)
        : id(id), weight(weight) {}
    uint32_t id;
    uint32_t weight;
};

struct ItemPreviewInfo
{
    ItemPreviewInfo(uint32_t serverId, const Position relativePosition)
        : serverId(serverId), relativePosition(relativePosition) {}

    uint32_t serverId;
    Position relativePosition;
};

class Brush
{
  public:
    Brush(std::string name);

    virtual ~Brush() = default;

    virtual void apply(MapView &mapView, const Position &position) = 0;

    virtual uint32_t iconServerId() const = 0;

    const std::string &name() const noexcept;

    virtual bool erasesItem(uint32_t serverId) const = 0;
    virtual BrushType type() const = 0;
    virtual std::vector<ItemPreviewInfo> previewInfo() const = 0;

    static Brush *getOrCreateRawBrush(uint32_t serverId);

    static GroundBrush *addGroundBrush(std::unique_ptr<GroundBrush> &&brush);
    static GroundBrush *addGroundBrush(GroundBrush &&brush);

    static int nextGroundBrushId();

    void setTileset(Tileset *tileset) noexcept;
    Tileset *tileset() const noexcept;

  protected:
    static vme_unordered_map<uint32_t, std::unique_ptr<Brush>> rawBrushes;
    static vme_unordered_map<uint32_t, std::unique_ptr<Brush>> groundBrushes;

    std::string _name;
    Tileset *_tileset = nullptr;
};