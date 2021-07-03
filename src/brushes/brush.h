#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "../creature.h"
#include "../debug.h"
#include "../graphics/texture_atlas.h"
#include "../position.h"
#include "../util.h"

struct Position;
class Tileset;
class MapView;
class RawBrush;
class GroundBrush;
class BorderBrush;
class DoodadBrush;
class CreatureBrush;
class ItemType;
class Brush;

#include <cstddef> // For std::ptrdiff_t
#include <iterator> // For std::forward_iterator_tag

enum class BrushType
{
    Creature,
    Doodad,
    Ground,
    Border,
    Raw,
};

struct BrushSearchResult
{
    std::unique_ptr<std::vector<Brush *>> matches;

    int rawCount = 0;
    int groundCount = 0;
    int doodadCount = 0;
    int creatureCount = 0;
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
    else if (rawType == "creature")
    {
        return BrushType::Creature;
    }
    else if (rawType == "border")
    {
        return BrushType::Border;
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

enum class BrushResourceType
{
    ItemType,
    Creature
};

struct BrushResource
{
    BrushResourceType type;
    uint32_t id;

    // Could be for example creature direction or item subtype
    uint8_t variant = 0;
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
    Brush(std::string name);

    virtual ~Brush() = default;

    virtual void apply(MapView &mapView, const Position &position, Direction direction) = 0;

    virtual const std::string getDisplayId() const = 0;

    const std::string &name() const noexcept;

    static BrushSearchResult search(std::string searchString);

    virtual std::vector<ThingDrawInfo> getPreviewTextureInfo(Direction direction = Direction::South) const = 0;

    virtual bool erasesItem(uint32_t serverId) const = 0;
    virtual BrushType type() const = 0;

    static Brush *getOrCreateRawBrush(uint32_t serverId);

    static GroundBrush *addGroundBrush(std::unique_ptr<GroundBrush> &&brush);
    static GroundBrush *addGroundBrush(GroundBrush &&brush);
    static GroundBrush *getGroundBrush(const std::string &id);

    static BorderBrush *addBorderBrush(BorderBrush &&brush);

    static DoodadBrush *addDoodadBrush(std::unique_ptr<DoodadBrush> &&brush);
    static DoodadBrush *addDoodadBrush(DoodadBrush &&brush);
    static DoodadBrush *getDoodadBrush(const std::string &id);

    static CreatureBrush *addCreatureBrush(std::unique_ptr<CreatureBrush> &&brush);
    static CreatureBrush *addCreatureBrush(CreatureBrush &&brush);
    static CreatureBrush *getCreatureBrush(const std::string &id);

    static const vme_unordered_map<uint32_t, std::unique_ptr<RawBrush>> &getRawBrushes();

    static int nextGroundBrushId();

    void setTileset(Tileset *tileset) noexcept;
    Tileset *tileset() const noexcept;

    static bool brushSorter(const Brush *leftBrush, const Brush *rightBrush);

    /* Should only be used to fill brush palettes
       TODO: Use iterator instead of returning a reference to the underlying map
    */
    static vme_unordered_map<std::string, std::unique_ptr<GroundBrush>> &getGroundBrushes();
    static vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> &getBorderBrushes();

  protected:
    // ServerId -> Brush
    static vme_unordered_map<uint32_t, std::unique_ptr<RawBrush>> rawBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<GroundBrush>> groundBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> borderBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<DoodadBrush>> doodadBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<CreatureBrush>> creatureBrushes;

    std::string _name;
    Tileset *_tileset = nullptr;
    static bool matchSorter(std::pair<int, Brush *> &lhs, const std::pair<int, Brush *> &rhs);
};