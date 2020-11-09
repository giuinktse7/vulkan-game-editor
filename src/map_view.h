#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include "camera.h"
#include "editor_action.h"
#include "map.h"
#include "position.h"
#include "selection.h"
#include "signal.h"
#include "util.h"

#include "history/history.h"
#include "history/history_action.h"

class MapHistory::ChangeItem;

class MapView : public Nano::Observer<>
{
public:
	MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action);
	MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action, std::shared_ptr<Map> map);
	~MapView();

	EditorAction &editorAction;

	MapHistory::History history;

	inline const Map *map() const noexcept;

	inline uint16_t mapWidth() const noexcept;
	inline uint16_t mapHeight() const noexcept;
	inline uint8_t mapDepth() const noexcept;

	void mousePressEvent(VME::MouseEvent event);
	void mouseMoveEvent(VME::MouseEvent event);
	void mouseReleaseEvent(VME::MouseEvent event);
	/*
		Escape the current action (for example when pressing the ESC key)
	*/
	void escapeEvent();

	void requestDraw();

	Selection &selection();

	/**
	 * Shorthand method for comitting actions within a group. Equivalent to:
	 * 
	 * history.startGroup(groupType);
   * f();
   * history.endGroup(groupType);
	 *
	 */
	void update(TransactionType groupType, std::function<void()> f);

	const Tile *getTile(const Position pos) const;
	Tile &getOrCreateTile(const Position pos);
	void insertTile(Tile &&tile);
	void removeTile(const Position pos);
	void modifyTile(const Position pos, std::function<void(Tile &)> f);

	void selectTopItem(const Tile &tile);
	void selectTopItem(const Position pos);
	void deselectTopItem(const Tile &tile);

	void selectTile(const Tile &tile);
	void selectTile(const Position &pos);

	void deselectTile(const Tile &tile);
	void deselectTile(const Position &pos);

	void clearSelection();
	bool isEmpty(const Position position) const;

	bool singleTileSelected() const;

	void setUnderMouse(bool underMouse);
	inline bool underMouse() const noexcept;

	Position cameraPosition() const noexcept;
	void setX(WorldPosition::value_type x);
	void setY(WorldPosition::value_type y);

	void translateX(WorldPosition::value_type x);
	void translateY(WorldPosition::value_type y);
	void translateCamera(WorldPosition delta);
	void translateZ(int z);
	/**
	 * Runs f once after the next draw.
	 */
	void waitForDraw(std::function<void()> f);

	void zoom(int delta);

	void moveSelection(const Position &offset);

	void addItem(const Position &position, uint16_t id);

	inline Position mouseGamePos() const;
	inline WorldPosition mouseWorldPos() const;

	/*
		Return the position on the map for the 'point'.
		A point (0, 0) corresponds to the map position (0, 0, mapViewZ).
	*/
	template <typename T>
	Position toPosition(util::Point<T> point) const;

	/* Note: The indices must be in descending order (std::greater), because
		otherwise the wrong items could be removed.
	*/
	void removeItems(const Position position, const std::set<size_t, std::greater<size_t>> &indices);
	void removeSelectedItems(const Tile &tile);
	void removeItems(const Tile &tile, std::function<bool(const Item &)> p);

	void zoomOut();
	void zoomIn();
	void resetZoom();

	void setViewportSize(int width, int height);

	float getZoomFactor() const noexcept;

	void undo();
	void redo();

	MapRegion mapRegion() const;

	inline uint32_t x() const noexcept;
	inline uint32_t y() const noexcept;
	inline int z() const noexcept;
	/*
		Synonym for z()
	*/
	inline int floor() const noexcept;

	const Camera::Viewport &getViewport() const noexcept;

	void deleteSelectedItems();

	util::Rectangle<int> getGameBoundingRect() const;

	void setDragStart(WorldPosition position);
	void setDragEnd(WorldPosition position);
	std::optional<std::pair<WorldPosition, WorldPosition>> getDragPoints() const;
	void endDragging(VME::ModifierKeys modifiers);
	bool isDragging() const;

	bool hasSelection() const;

	inline ScreenPosition mousePos() const;

	inline static bool isInstance(MapView *pointer);

	bool draggingWithSubtract() const;

	bool inDragRegion(Position pos) const;

	template <auto mem_ptr, typename T>
	void onViewportChanged(T *instance);

	template <auto mem_ptr, typename T>
	void onDrawRequested(T *instance);

	std::optional<Region2D<WorldPosition>> dragRegion;

private:
	friend class MapHistory::ChangeItem;

	Nano::Signal<void(const Camera::Viewport &)> viewportChange;
	Nano::Signal<void()> drawRequest;

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

	Camera camera;

	bool canRender = false;

	/**
	 * NOTE: Do not use this for anything except to request a draw from mouseEvent.
	 * Use mousePos() instead.
	 */
	Position _previousMouseGamePos;

	bool _underMouse = false;

	Tile deepCopyTile(const Position position) const;

	void selectRegion(const Position &from, const Position &to);
	void removeItemsInRegion(const Position &from, const Position &to, std::function<bool(const Item &)> predicate);
	void fillRegion(const Position &from, const Position &to, uint32_t serverId);

	void cameraViewportChangedEvent();

	void setPanOffset(MouseAction::Pan &action, const ScreenPosition &offset);
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
	return camera.viewport();
}

inline Position MapView::cameraPosition() const noexcept
{
	return camera.position();
}

inline ScreenPosition MapView::mousePos() const
{
	return uiUtils->mouseScreenPosInView();
}

inline Position MapView::mouseGamePos() const
{
	return mousePos().worldPos(*this).mapPos().floor(floor());
}

inline WorldPosition MapView::mouseWorldPos() const
{
	return mousePos().worldPos(*this);
}

template <typename T>
inline Position MapView::toPosition(util::Point<T> point) const
{
	return ScreenPosition(point.x(), point.y()).toPos(*this);
}

inline uint32_t MapView::x() const noexcept
{
	return static_cast<uint32_t>(camera.worldPosition().x);
}

inline uint32_t MapView::y() const noexcept
{
	return static_cast<uint32_t>(camera.worldPosition().y);
}

inline int MapView::z() const noexcept
{
	return camera.z();
}

inline int MapView::floor() const noexcept
{
	return z();
}

inline bool MapView::underMouse() const noexcept
{
	return _underMouse;
}

template <auto mem_ptr, typename T>
void MapView::onViewportChanged(T *instance)
{
	viewportChange.connect<mem_ptr>(instance);
}

template <auto mem_ptr, typename T>
void MapView::onDrawRequested(T *instance)
{
	drawRequest.connect<mem_ptr>(instance);
}