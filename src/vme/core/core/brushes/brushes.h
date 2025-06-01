#pragma once

#include <memory>
#include <optional>
#include <string>

#include "../tile.h"
#include "../util.h"
#include "border_brush.h"
#include "brush.h"
#include "creature_brush.h"
#include "doodad_brush.h"
#include "ground_brush.h"
#include "mountain_brush.h"
#include "raw_brush.h"
#include "wall_brush.h"

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

class Brushes
{
  public:
    static BrushSearchResult search(const std::string &searchString);

    static std::optional<BrushType> parseBrushType(const std::string &brushType);

    static RawBrush *getOrCreateRawBrush(uint32_t serverId);

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

    static bool brushSorter(const Brush *leftBrush, const Brush *rightBrush);

    static RawBrush *getRawBrush(const Tile &tile);
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

  private:
    static BrushShape *_brushShape;
};
