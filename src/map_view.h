#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

#include "camera.h"
#include "editor_action.h"
#include "history/history.h"
#include "history/history_action.h"
#include "item_location.h"
#include "map.h"
#include "position.h"
#include "selection.h"
#include "signal.h"
#include "tile.h"
#include "util.h"

class GroundBrush;
class MountainBrush;

namespace MapHistory
{
    class ChangeItem;
}

class MapView;

struct ItemDropEvent
{
    MapView *mapView;
};

class MapView
{

  public:
    enum ViewOption : int
    {
        None = 0,
        ShadeLowerFloors = 1 << 0
    };

    struct Overlay
    {
        Item *draggedItem = nullptr;
    };

    MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action);
    MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action, std::shared_ptr<Map> map);
    ~MapView();

    // Only for testing
    void perfTest();
    void testBordering();

    inline const Map *map() const noexcept;

    inline uint16_t mapWidth() const noexcept;
    inline uint16_t mapHeight() const noexcept;
    inline uint8_t mapDepth() const noexcept;

    void dragEnterEvent();
    void dragLeaveEvent();
    void mousePressEvent(VME::MouseEvent event);
    void mouseMoveEvent(VME::MouseEvent event);
    void mouseReleaseEvent(VME::MouseEvent event);
    void itemDropEvent(const ItemDropEvent &event);

    /*
		Escape the current action (for example when pressing the ESC key)
	*/
    void escapeEvent();
    void requestDraw();
    void requestMinimapDraw();

    /**
	 * Shorthand method for comitting actions within a group. Equivalent to:
	 * 
	 * history.startGroup(groupType);
   * f();
   * history.endGroup(groupType);
	 *
	 */
    void commitTransaction(TransactionType groupType, std::function<void()> f);

    void beginTransaction(TransactionType transactionType);
    void endTransaction(TransactionType transactionType);

    Tile *getTile(const Position pos) const;
    Tile *getTile(const Position pos);

    bool hasTile(const Position &pos) const noexcept;
    void createTile(const Position pos);
    Tile &getOrCreateTile(const Position pos);
    void insertTile(Tile &&tile);
    void insertTile(std::unique_ptr<Tile> &&tile);
    void removeTile(const Position pos);
    void modifyTile(const Position pos, std::function<void(Tile &)> f);

    void selectTopItem(const Tile &tile);
    void selectTopItem(const Position pos);
    void deselectTopItem(const Tile &tile);

    void selectTopThing(const Position &position);

    void selectTile(const Tile &tile);
    void selectTile(const Position &pos);

    void deselectTile(const Tile &tile);
    void deselectTile(const Position &pos);

    void clearSelection();

    void setUnderMouse(bool underMouse);

    void setX(WorldPosition::value_type x);
    void setY(WorldPosition::value_type y);

    void floorUp();
    void floorDown();
    void setFloor(int floor);

    const Camera &camera() const noexcept;

    void translateCamera(WorldPosition delta);

    /**
	 * Runs f once after the next draw.
	 */
    void waitForDraw(std::function<void()> f);

    void zoom(int delta);

    void moveItem(const Tile &fromTile, const Position toPosition, Item *item);

    void setSpawnInterval(Creature *creature, int spawnInterval);

    void setSubtype(Item *item, uint8_t count);
    void setItemActionId(Item *item, uint16_t actionId);
    void setText(Item *item, const std::string &text);

    void moveFromMapToContainer(Tile &tile, Item *item, ContainerLocation &containerInfo);
    void moveFromContainerToMap(ContainerLocation &moveInfo, Tile &tile);
    void moveFromContainerToContainer(ContainerLocation &from, ContainerLocation &to);

    void moveSelection(const Position &offset);

    /**
     * Places the border correctly based on border z-order
     */
    void addBorder(const Position &position, uint32_t id, uint32_t zOrder);
    void addItem(const Position &position, uint32_t id);
    void addItem(const Position &pos, Item &&item, bool onBlocking = true);
    void addItem(Tile &tile, Item &&item);
    void setGround(Tile &tile, Item &&ground, bool clearBorders = false);
    void replaceItemByServerId(Tile &tile, uint32_t oldServerId, uint32_t newServerId);

    void setBottomItem(const Position &position, Item &&item);
    void setBottomItem(const Tile &tile, Item &&item);

    void addCreature(const Position &pos, Creature &&creature);

    /* Note: The indices must be in descending order (std::greater), because
		otherwise the wrong items could be removed.
	*/
    void removeItems(const Position position, const std::set<size_t, std::greater<size_t>> &indices);
    void removeItems(const Position &position, std::function<bool(const Item &)> predicate);
    void removeItems(const Tile &tile, std::function<bool(const Item &)> predicate);
    void removeSelectedItems(const Tile &tile);
    void removeItemsWithBorderize(const Tile &tile, std::function<bool(const Item &)> p);
    void removeItem(Tile &tile, Item *item);
    void removeItem(Tile &tile, std::function<bool(const Item &)> p);

    void zoomOut();
    void zoomIn();
    void resetZoom();

    void setViewportSize(int width, int height);

    void undo();
    void redo();

    void toggleViewOption(ViewOption option);
    void setViewOption(ViewOption option, bool value);
    inline ViewOption viewOptions() const noexcept;
    inline bool hasOption(ViewOption option) const noexcept;

    MapRegion mapRegion(int padTilesX = 0, int padTilesY = 0) const;
    MapRegion mapRegion(int8_t floor) const;

    inline uint32_t x() const noexcept;
    inline uint32_t y() const noexcept;
    inline int z() const noexcept;
    /* Synonym for z() */
    inline int floor() const noexcept;

    void deleteSelectedItems();

    void setDragStart(WorldPosition position);
    void setDragEnd(WorldPosition position);
    void endDragging(VME::ModifierKeys modifiers);

    void startItemDrag(Tile *tile, Item *item);

    std::shared_ptr<Item> dropItem(Tile *tile, Item *item);

    bool singleTileSelected() const;
    const Tile *singleSelectedTile() const;
    Tile *singleSelectedTile();
    bool singleThingSelected() const;

    void rotateBrush(bool forwards = true);

    /**
	 * Returns the only selected item if there is **exactly one** selected item.
	 * Returns nullptr otherwise.
	 */
    const Item *singleSelectedItem() const;
    /**
	 * Returns the only selected item if there is **exactly one** selected item.
	 * Returns nullptr otherwise.
	 */
    Item *singleSelectedItem();

    /**
     * Returns the only selected thing (or std::monostate if there is zero or more than one selected thing).
     */
    TileThing singleSelectedThing();

    bool isDragging() const;
    bool hasSelection() const;
    bool draggingWithSubtract() const;
    bool inDragRegion(Position pos) const;
    inline bool underMouse() const noexcept;
    inline bool prevUnderMouse() const noexcept;

    std::optional<Position> getLastBrushDragPosition() const noexcept;
    std::optional<TileQuadrant> getLastClickedTileQuadrant() const noexcept;
    std::optional<TileQuadrant> getMouseDownTileQuadrant() const noexcept;

    bool isEmpty(const Position position) const;

    Position cameraPosition() const noexcept;
    inline Position mouseGamePos() const;
    inline WorldPosition mouseWorldPos() const;
    /*
		Return the position on the map for the 'point'.
		A point (0, 0) corresponds to the map position (0, 0, mapViewZ).
	*/
    template <typename T>
    Position toPosition(util::Point<T> point) const;

    Selection &selection();
    const Selection &selection() const;
    const Camera::Viewport &getViewport() const noexcept;
    util::Rectangle<int> getGameBoundingRect() const;
    std::optional<std::pair<WorldPosition, WorldPosition>> getDragPoints() const;
    float getZoomFactor() const noexcept;

    inline ScreenPosition mousePos() const;

    inline static bool isInstance(MapView *pointer);

    Overlay &overlay() noexcept;

    template <auto MemberFunction, typename T>
    void onViewportChanged(T *instance);

    template <auto MemberFunction, typename T>
    void onDrawRequested(T *instance);

    template <auto MemberFunction, typename T>
    void onDrawMinimapRequest(T *instance);

    template <auto MemberFunction, typename T>
    void onMapItemDragStart(T *instance);

    template <auto MemberFunction, typename T>
    void onSelectedTileThingClicked(T *instance);

    template <auto MemberFunction, typename T>
    void onUndoRedo(T *instance);

    int getBrushVariation() const noexcept;

    static Direction getDirection(int variation);

    bool isValidPos(const Position &position) const;

    EditorAction &editorAction;
    MapHistory::History history;

    std::optional<Region2D<WorldPosition>> dragRegion;

  private:
    friend class MapHistory::ChangeItem;

    void borderize(const Position &position);

    Tile deepCopyTile(const Position position) const;

    void selectRegion(const Position &from, const Position &to);
    void removeItemsInRegion(const Position &from, const Position &to, std::function<bool(const Item &)> predicate);
    void fillRegion(const Position &from, const Position &to, uint32_t serverId);
    void fillRegion(const Position &from, const Position &to, std::function<uint32_t()> itemSupplier);
    void fillRegionByGroundBrush(const Position &from, const Position &to, GroundBrush *brush);
    void fillRegionByMountainBrush(const Position &from, const Position &to, MountainBrush *brush);
    void endCurrentAction(VME::ModifierKeys modifiers);

    void selectTopThing(const Position &position, bool isNewSelection);

    void cameraViewportChangedEvent();

    void setPanOffset(MouseAction::Pan &action, const ScreenPosition &offset);

    void setLastClickedTileQuadrant(const WorldPosition worldPos);

    /**
 		*	Keeps track of all MapView instances. This is necessary for QT to
		* validate MapView pointers in a QVariant.
		*
		* See:
		* QtUtil::associatedMapView
	*/
    static std::unordered_set<MapView *> instances;

    std::shared_ptr<Map> _map;
    Selection _selection;

    std::unique_ptr<UIUtils> uiUtils;

    Camera _camera;

    ViewOption _viewOptions = ViewOption::None;

    bool canRender = false;

    Position _previousMouseGamePos;
    TileQuadrant _previousMouseMoveTileQuadrant = TileQuadrant::TopLeft;
    std::optional<TileQuadrant> mouseDownTileQuadrant;

    /**
     * When dragging with a brush, this keeps track of the latest position that the brush was applied to.
     */
    std::optional<Position> lastBrushDragPosition;

    /**
     * 
     */
    std::optional<TileQuadrant> lastClickedTileQuadrant;

    Overlay _overlay;

    bool _prevUnderMouse = false;
    bool _underMouse = false;

    Nano::Signal<void(const Camera::Viewport &)> viewportChange;
    Nano::Signal<void(Tile *tile, Item *item)> mapItemDragStart;
    Nano::Signal<void(MapView *mapView, const Tile *tile, TileThing tileThing)> selectedTileThingClicked;
    Nano::Signal<void()> drawRequest;
    Nano::Signal<void()> drawMinimapRequest;
    Nano::Signal<void()> undoRedoPerformed;
};

inline const Map *MapView::map() const noexcept
{
    return _map.get();
}

inline uint16_t MapView::mapWidth() const noexcept
{
    return _map->width();
}

inline uint16_t MapView::mapHeight() const noexcept
{
    return _map->height();
}

inline uint8_t MapView::mapDepth() const noexcept
{
    return _map->depth();
}

inline Selection &MapView::selection()
{
    return _selection;
}

inline const Selection &MapView::selection() const
{
    return _selection;
}

inline Tile MapView::deepCopyTile(const Position position) const
{
    return _map->getTile(position)->deepCopy();
}

inline bool MapView::isInstance(MapView *pointer)
{
    return instances.find(pointer) != instances.end();
}

inline std::ostream &operator<<(std::ostream &os, const util::Rectangle<int> &rect)
{
    os << "{ x1=" << rect.x1 << ", y1=" << rect.y1 << ", x2=" << rect.x2 << ", y2=" << rect.y2 << "}";
    return os;
}

inline const Camera::Viewport &MapView::getViewport() const noexcept
{
    return _camera.viewport();
}

inline Position MapView::cameraPosition() const noexcept
{
    return _camera.position();
}

inline ScreenPosition MapView::mousePos() const
{
    return uiUtils->mouseScreenPosInView();
}

inline Position MapView::mouseGamePos() const
{
    return mousePos().toPos(*this);
}

inline WorldPosition MapView::mouseWorldPos() const
{
    return mousePos().worldPos(*this);
}

inline MapView::Overlay &MapView::overlay() noexcept
{
    return _overlay;
}

template <typename T>
inline Position MapView::toPosition(util::Point<T> point) const
{
    return ScreenPosition(point.x(), point.y()).toPos(*this);
}

inline uint32_t MapView::x() const noexcept
{
    return static_cast<uint32_t>(_camera.worldPosition().x);
}

inline uint32_t MapView::y() const noexcept
{
    return static_cast<uint32_t>(_camera.worldPosition().y);
}

inline int MapView::z() const noexcept
{
    return _camera.z();
}

inline int MapView::floor() const noexcept
{
    return z();
}

inline bool MapView::underMouse() const noexcept
{
    return _underMouse;
}

inline bool MapView::prevUnderMouse() const noexcept
{
    return _prevUnderMouse;
}

inline MapView::ViewOption MapView::viewOptions() const noexcept
{
    return _viewOptions;
}

inline bool MapView::hasOption(ViewOption option) const noexcept
{
    return _viewOptions & option;
}

template <auto MemberFunction, typename T>
void MapView::onViewportChanged(T *instance)
{
    viewportChange.connect<MemberFunction>(instance);
}

template <auto MemberFunction, typename T>
void MapView::onDrawRequested(T *instance)
{
    drawRequest.connect<MemberFunction>(instance);
}

template <auto MemberFunction, typename T>
void MapView::onDrawMinimapRequest(T *instance)
{
    drawMinimapRequest.connect<MemberFunction>(instance);
}

template <auto MemberFunction, typename T>
void MapView::onMapItemDragStart(T *instance)
{
    mapItemDragStart.connect<MemberFunction>(instance);
}

template <auto MemberFunction, typename T>
void MapView::onSelectedTileThingClicked(T *instance)
{
    selectedTileThingClicked.connect<MemberFunction>(instance);
}

template <auto MemberFunction, typename T>
void MapView::onUndoRedo(T *instance)
{
    undoRedoPerformed.connect<MemberFunction>(instance);
}

VME_ENUM_OPERATORS(MapView::ViewOption)
