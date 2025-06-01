#pragma once

#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include "../creature.h"
#include "../lazy_object.h"
#include "../position.h"

struct Position;
class Tileset;
class MapView;
class RawBrush;
class Map;
class Tile;
class GroundBrush;
class ItemType;
class Brush;

class BrushShape
{
  public:
    virtual ~BrushShape() = default;
    [[nodiscard]] virtual std::unordered_set<Position> getRelativePositions() const noexcept = 0;
};

class RectangularBrushShape : public BrushShape
{
  public:
    RectangularBrushShape(uint16_t width, uint16_t height);
    std::unordered_set<Position> getRelativePositions() const noexcept override;

  private:
    uint16_t width;
    uint16_t height;
};

class CircularBrushShape : public BrushShape
{
  public:
    CircularBrushShape(uint16_t radius);
    std::unordered_set<Position> getRelativePositions() const noexcept override;

  private:
    static std::unordered_set<Position> Size3x3;
    uint16_t radius;
};

struct ExpandedTileBlock
{
    int x = 0;
    int y = 0;
};

enum class BrushType
{
    Creature,
    Doodad,
    Ground,
    Border,
    Raw,
    Wall,
    Mountain
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

struct DrawItemType
{
    DrawItemType(uint32_t serverId, Position relativePosition);
    DrawItemType(ItemType *itemType, Position relativePosition);

    ItemType *itemType;
    Position relativePosition;
    uint8_t subtype;
};

struct DrawCreatureType
{
    DrawCreatureType(const CreatureType *creatureType, Position relativePosition, Direction direction = Direction::South)
        : creatureType(creatureType), relativePosition(relativePosition), direction(direction) {}

    const CreatureType *creatureType;
    Position relativePosition;
    Direction direction;
};

using ThingDrawInfo = std::variant<DrawItemType, DrawCreatureType>;

class Brush
{
  public:
    struct LazyGround : public LazyObject<GroundBrush *>
    {
        LazyGround(GroundBrush *brush);
        LazyGround(const std::string &groundBrushId);
    };

    Brush(std::string name);

    virtual ~Brush() = default;

    virtual void applyWithoutBorderize(MapView &mapView, const Position &position);
    virtual void apply(MapView &mapView, const Position &position) = 0;
    virtual void erase(MapView &mapView, const Position &position) = 0;

    [[nodiscard]] virtual std::string getDisplayId() const = 0;

    const std::string &name() const noexcept;

    virtual std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const = 0;
    virtual void updatePreview(int variation);
    virtual int variationCount() const;

    virtual bool erasesItem(uint32_t serverId) const = 0;
    virtual BrushType type() const = 0;

    void setTileset(Tileset *tileset) noexcept;
    Tileset *tileset() const noexcept;

  protected:
    std::string _name;
    Tileset *_tileset = nullptr;
};
