#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "../debug.h"
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

inline std::optional<BrushType> parseBrushType(const std::string &rawType)
{
    if (rawType == "ground")
    {
        return BrushType::Ground;
    }
    else if (rawType == "raw")
    {
        return BrushType::Raw;
    }
    else if (rawType == "doodad")
    {
        return BrushType::Doodad;
    }
    else
    {
        return std::nullopt;
    }
}

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

    static std::unique_ptr<std::vector<Brush *>> search(std::string searchString);

    virtual bool erasesItem(uint32_t serverId) const = 0;
    virtual BrushType type() const = 0;
    virtual std::vector<ItemPreviewInfo> previewInfo() const = 0;

    static Brush *getOrCreateRawBrush(uint32_t serverId);

    static GroundBrush *addGroundBrush(std::unique_ptr<GroundBrush> &&brush);
    static GroundBrush *addGroundBrush(GroundBrush &&brush);
    static GroundBrush *getGroundBrush(const std::string &name);

    static const vme_unordered_map<uint32_t, std::unique_ptr<Brush>> &getRawBrushes();

    static int nextGroundBrushId();

    void setTileset(Tileset *tileset) noexcept;
    Tileset *tileset() const noexcept;

  protected:
    static vme_unordered_map<uint32_t, std::unique_ptr<Brush>> rawBrushes;

    static vme_unordered_map<std::string, std::unique_ptr<Brush>> groundBrushes;

    std::string _name;
    Tileset *_tileset = nullptr;

  private:
    static std::vector<std::pair<int, Brush *>> searchWithScore(std::string searchString);
};