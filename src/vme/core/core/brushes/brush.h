#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

#include "../creature.h"
#include "../debug.h"
#include "../graphics/texture_atlas.h"
#include "../lazy_object.h"
#include "../position.h"
#include "../tile_cover.h"
#include "../util.h"

struct Position;
class Tileset;
class MapView;
class RawBrush;
class GroundBrush;
class BorderBrush;
class WallBrush;
class DoodadBrush;
class CreatureBrush;
class MountainBrush;
class Map;
class Tile;
class ItemType;
class Brush;

class BrushShape
{
  public:
    virtual std::unordered_set<Position> getRelativePositions() const noexcept = 0;
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

struct BrushSearchResult
{
    std::unique_ptr<std::vector<Brush *>> matches;

    int rawCount = 0;
    int groundCount = 0;
    int doodadCount = 0;
    int creatureCount = 0;
    int borderCount = 0;
    int wallCount = 0;
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
    struct LazyGround : public LazyObject<GroundBrush *>
    {
        LazyGround(GroundBrush *brush);
        LazyGround(std::string groundBrushId);
    };

    Brush(std::string name);

    virtual ~Brush() = default;

    virtual void applyWithoutBorderize(MapView &mapView, const Position &position);
    virtual void apply(MapView &mapView, const Position &position) = 0;
    virtual void erase(MapView &mapView, const Position &position) = 0;

    virtual const std::string getDisplayId() const = 0;

    const std::string &name() const noexcept;

    static BrushSearchResult search(std::string searchString);

    virtual std::vector<ThingDrawInfo> getPreviewTextureInfo(int variation) const = 0;
    virtual void updatePreview(int variation);
    virtual int variationCount() const;

    static std::optional<BrushType> parseBrushType(std::string s);

    virtual bool erasesItem(uint32_t serverId) const = 0;
    virtual BrushType type() const = 0;

    static Brush *getOrCreateRawBrush(uint32_t serverId);

    static GroundBrush *addGroundBrush(std::unique_ptr<GroundBrush> &&brush);
    static GroundBrush *addGroundBrush(GroundBrush &&brush);
    static GroundBrush *getGroundBrush(const std::string &id);

    static BorderBrush *addBorderBrush(BorderBrush &&brush);
    static BorderBrush *getBorderBrush(const std::string &id);

    static DoodadBrush *addDoodadBrush(std::unique_ptr<DoodadBrush> &&brush);
    static DoodadBrush *addDoodadBrush(DoodadBrush &&brush);
    static DoodadBrush *getDoodadBrush(const std::string &id);

    static MountainBrush *addMountainBrush(std::unique_ptr<MountainBrush> &&brush);
    static MountainBrush *addMountainBrush(MountainBrush &&brush);
    static MountainBrush *getMountainBrush(const std::string &id);

    static WallBrush *addWallBrush(WallBrush &&brush);

    static CreatureBrush *addCreatureBrush(std::unique_ptr<CreatureBrush> &&brush);
    static CreatureBrush *addCreatureBrush(CreatureBrush &&brush);
    static CreatureBrush *getCreatureBrush(const std::string &id);

    static const vme_unordered_map<uint32_t, std::unique_ptr<RawBrush>> &getRawBrushes();

    void setTileset(Tileset *tileset) noexcept;
    Tileset *tileset() const noexcept;

    static bool brushSorter(const Brush *leftBrush, const Brush *rightBrush);

    static GroundBrush *getGroundBrush(const Tile &tile);
    static BorderBrush *getBorderBrush(const Tile &tile);
    static DoodadBrush *getDoodadBrush(const Tile &tile);
    static WallBrush *getWallBrush(const Tile &tile);
    static CreatureBrush *getCreatureBrush(const Tile &tile);
    static MountainBrush *getMountainBrush(const Tile &tile);

    /* Should only be used to fill brush palettes
       TODO: Use iterator instead of returning a reference to the underlying map
    */
    static vme_unordered_map<std::string, std::unique_ptr<GroundBrush>> &getGroundBrushes();
    static vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> &getBorderBrushes();
    static vme_unordered_map<std::string, std::unique_ptr<WallBrush>> &getWallBrushes();
    static vme_unordered_map<std::string, std::unique_ptr<DoodadBrush>> &getDoodadBrushes();
    static vme_unordered_map<std::string, std::unique_ptr<MountainBrush>> &getMountainBrushes();

    static BrushShape &brushShape() noexcept;

    /**
     * Takes ownership of the BrushShape pointer
     */
    static void setBrushShape(BrushShape *brushShape) noexcept;

  protected:
    static bool matchSorter(std::pair<int, Brush *> &lhs, const std::pair<int, Brush *> &rhs);

    // ServerId -> Brush
    static vme_unordered_map<uint32_t, std::unique_ptr<RawBrush>> rawBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<GroundBrush>> groundBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<BorderBrush>> borderBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<WallBrush>> wallBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<DoodadBrush>> doodadBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<CreatureBrush>> creatureBrushes;

    // BrushId -> Brush
    static vme_unordered_map<std::string, std::unique_ptr<MountainBrush>> mountainBrushes;

    std::string _name;
    Tileset *_tileset = nullptr;

  private:
    static BrushShape *_brushShape;
};

struct BorderNeighborMap
{
    BorderNeighborMap(const Position &position, BorderBrush *brush, const Map &map);
    TileCover at(int x, int y) const;
    TileCover &at(int x, int y);
    TileCover &center();
    void set(int x, int y, TileCover tileCover);

    bool isExpanded(int x, int y) const;
    bool hasExpandedCover() const noexcept;
    void addExpandedCover(int x, int y);

    std::vector<ExpandedTileBlock> expandedCovers;

  private:
    int index(int x, int y) const;
    TileCover getTileCoverAt(BorderBrush *brush, const Map &map, const Position position) const;

    std::array<TileCover, 25> data;
};

struct BorderData
{
    BorderData(std::array<uint32_t, 12> borderIds);
    BorderData(std::array<uint32_t, 12> borderIds, GroundBrush *centerBrush);

    bool is(uint32_t serverId, BorderType borderType) const;
    std::optional<uint32_t> getServerId(BorderType borderType) const noexcept;
    BorderType getBorderType(uint32_t serverId) const;
    std::array<uint32_t, 12> getBorderIds() const;
    GroundBrush *centerBrush() const;

    void setCenterGroundId(const std::string &id);

    void setExtraBorderIds(vme_unordered_map<uint32_t, BorderType> &&extraIds);
    const vme_unordered_map<uint32_t, BorderType> *getExtraBorderIds() const;

  private:
    std::array<uint32_t, 12> borderIds = {};

    /**
     * Extras border ids that also count as tile cover for this border
     */
    std::unique_ptr<vme_unordered_map<uint32_t, BorderType>> extraIds;

    std::optional<Brush::LazyGround> _centerBrush;
};