#pragma once

#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include "camera.h"
#include "input.h"
#include "map.h"
#include "position.h"
#include "selection.h"
#include "signal.h"
#include "util.h"

#include "history/history.h"

class MapAction;

class MapView : public Nano::Observer<>
{
public:
	MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action);
	MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action, std::shared_ptr<Map> map);
	~MapView();

	EditorAction &editorAction;

	MapHistory::History history;

	inline Map *map() const;

	void mousePressEvent(VME::MouseEvent event);
	void mouseMoveEvent(VME::MouseEvent event);
	void mouseReleaseEvent(VME::MouseEvent event);
	/*
		Escape the current action (for example when pressing the ESC key)
	*/
	void escapeEvent();

	Selection &selection();

	Tile *getTile(const Position pos) const;
	Tile &getOrCreateTile(const Position pos);
	void insertTile(Tile &&tile);
	void removeTile(const Position pos);
	void selectTopItem(Tile &tile);
	void selectTopItem(const Position pos);
	void deselectTopItem(Tile &tile);
	void selectAll(Tile &tile);
	void clearSelection();
	bool hasSelectionMoveOrigin() const;
	bool isEmpty(const Position position) const;

	void setUnderMouse(bool underMouse);
	inline bool underMouse() const noexcept;

	void setCameraPosition(Position position);
	Position cameraPosition() const noexcept;
	void setX(WorldPosition::value_type x);
	void setY(WorldPosition::value_type y);

	void translateX(WorldPosition::value_type x);
	void translateY(WorldPosition::value_type y);
	void translateCamera(WorldPosition delta);
	void translateZ(int z);

	void zoom(int delta);

	void finishMoveSelection();

	void addItem(const Position position, uint16_t id);

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
	template <class UnaryPredicate>
	inline void removeItems(const Tile &tile, UnaryPredicate p);

	void zoomOut();
	void zoomIn();
	void resetZoom();

	void setViewportSize(int width, int height);

	float getZoomFactor() const noexcept;

	const MapPosition worldToMapPos(WorldPosition worldPos) const;
	const Position screenToMapPos(ScreenPosition screenPos) const;
	const MapPosition windowToMapPos(ScreenPosition screenPos) const;
	const uint32_t windowToMapPos(int windowPos) const;
	const uint32_t mapToWorldPos(uint32_t mapPos) const;

	void undo()
	{
		history.undoLast();
	}

	MapRegion mapRegion() const;

	inline uint32_t x() const noexcept;
	inline uint32_t y() const noexcept;
	inline int z() const noexcept;
	/*
		Synonym for z()
	*/
	inline int floor() const noexcept;

	const Camera::Viewport &getViewport() const noexcept;

	void deleteSelection();
	void updateSelection(const Position pos);

	util::Rectangle<int> getGameBoundingRect() const;

	void setDragStart(WorldPosition position);
	void setDragEnd(WorldPosition position);
	std::optional<std::pair<WorldPosition, WorldPosition>> getDragPoints() const;
	void endDragging(VME::ModifierKeys modifiers);
	bool isDragging() const;

	bool hasSelection() const;

	inline ScreenPosition mousePos() const;

	/*
	TODO: These should probably be private, but they are needed by MapHistory
		Returns the old tile at the location of the tile.
	*/
	std::unique_ptr<Tile> setTileInternal(Tile &&tile);
	std::unique_ptr<Tile> removeTileInternal(const Position position);
	void removeSelectionInternal(Tile *tile);

	inline static bool isInstance(MapView *pointer);

	template <typename T>
	T *currentAction() const;

	bool draggingWithSubtract() const;

	bool inDragRegion(Position pos) const;

	template <auto mem_ptr, typename T>
	void onViewportChanged(T *instance);

	template <auto mem_ptr, typename T>
	void onDrawRequested(T *instance);

	std::optional<Region2D<WorldPosition>> dragRegion;

	void disconnectAll();

private:
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
	Position _drawRequestMousePos;

	bool _underMouse = false;

	Tile deepCopyTile(const Position position) const;

	MapHistory::Action newAction(MapHistory::ActionType actionType) const;

	void cameraViewportChangedEvent();
};

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

template <class UnaryPredicate>
inline void MapView::removeItems(const Tile &tile, UnaryPredicate predicate)
{

	Tile newTile = tile.deepCopy();
	if (newTile.removeItemsIf(predicate) > 0)
	{
		MapHistory::Action action(MapHistory::ActionType::ModifyTile);
		action.addChange(MapHistory::SetTile(std::move(newTile)));

		history.commit(std::move(action));
	}
}

inline const Camera::Viewport &MapView::getViewport() const noexcept
{
	return camera.viewport();
}

inline Map *MapView::map() const
{
	return _map.get();
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