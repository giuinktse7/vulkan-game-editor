#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <set>
#include <variant>

#include "map.h"
#include "camera.h"
#include "position.h"
#include "util.h"
#include "input.h"
#include "selection.h"

#include "history/history.h"

class MapAction;

struct Viewport
{
	Viewport();
	int width;
	int height;
	float zoom;

	WorldPosition offset;
};

class MapView
{
public:
	class Observer
	{
	public:
		Observer(MapView *target = nullptr);
		~Observer();
		virtual void viewportChanged(const Viewport &viewport)
		{
			// Empty
		}

		MapView *target;

	private:
		friend class ::MapView;
		enum class ChangeType
		{
			Viewport,
		};
	};

	MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action);
	MapView(std::unique_ptr<UIUtils> uiUtils, EditorAction &action, std::shared_ptr<Map> map);
	~MapView();

	EditorAction &editorAction;

	MapHistory::History history;

	Selection selection;

	inline Map *map() const
	{
		return _map.get();
	}

	void mousePressEvent(VME::MouseEvent event);
	void mouseMoveEvent(VME::MouseEvent event);
	void mouseReleaseEvent(VME::MouseEvent event);
	/*
		Escape the current action (for example when pressing the ESC key)
	*/
	void escapeEvent();

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
	bool isEmpty(Position position) const;

	void setCameraPosition(WorldPosition position);
	void setX(long x);
	void setY(long y);

	void translateX(long x);
	void translateY(long y);
	void translateCamera(WorldPosition delta);
	void translateZ(int z);

	void zoom(int delta);

	void finishMoveSelection(const Position moveDestination);

	void addItem(const Position position, uint16_t id);

	inline Position mouseGamePos() const
	{
		return mousePos().worldPos(*this).mapPos().floor(getFloor());
	}

	inline WorldPosition mouseWorldPos() const
	{
		return mousePos().worldPos(*this);
	}

	/*
		Return the position on the map for the 'point'.
		A point (0, 0) corresponds to the map position (0, 0, mapViewZ).
	*/
	template <typename T>
	Position toPosition(util::Point<T> point) const
	{
		return ScreenPosition(point.x(), point.y()).toPos(*this);
	}

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

	void updateViewport();
	void setViewportSize(int width, int height);

	float getZoomFactor() const noexcept;

	const MapPosition worldToMapPos(WorldPosition worldPos) const;
	const Position screenToMapPos(ScreenPosition screenPos) const;
	const MapPosition windowToMapPos(ScreenPosition screenPos) const;
	const uint32_t windowToMapPos(int windowPos) const;
	const uint32_t mapToWorldPos(uint32_t mapPos) const;

	inline int z() const noexcept
	{
		return static_cast<uint32_t>(camera.floor);
	}

	void undo()
	{
		history.undoLast();
	}

	/*
		Synonym for z()
	*/
	inline int getFloor() const noexcept
	{
		return z();
	}

	inline uint32_t x() const noexcept
	{
		return static_cast<uint32_t>(camera.position().x);
	}

	inline uint32_t y() const noexcept
	{
		return static_cast<uint32_t>(camera.position().y);
	}

	const Viewport &getViewport() const noexcept
	{
		return viewport;
	}

	void deleteSelectedItems();
	void updateSelection(const Position pos);

	util::Rectangle<int> getGameBoundingRect() const;

	void setDragStart(WorldPosition position);
	void setDragEnd(WorldPosition position);
	std::optional<std::pair<WorldPosition, WorldPosition>> getDragPoints() const
	{
		if (!dragState.has_value())
			return {};

		std::pair<WorldPosition, WorldPosition> result;
		result.first = dragState.value().from;
		result.second = dragState.value().to;
		return result;
	}
	void endDragging(VME::ModifierKeys modifiers);
	bool isDragging() const;

	bool hasSelection() const;

	enum class PanType
	{
		Horizontal,
		Vertical
	};

	struct PanEvent
	{
		PanType type;
		long value;
	};

	void panEvent(PanEvent event);

	inline ScreenPosition mousePos() const
	{
		return uiUtils->mouseScreenPosInView();
	}

	void addObserver(MapView::Observer *observer);
	void removeObserver(MapView::Observer *observer);
	void notifyObservers(MapView::Observer::ChangeType changeType) const;

	/*
	TODO: These should probably be private, but they are needed by MapHistory
		Returns the old tile at the location of the tile.
	*/
	std::unique_ptr<Tile> setTileInternal(Tile &&tile);
	std::unique_ptr<Tile> removeTileInternal(const Position position);
	void removeSelectionInternal(Tile *tile);

	inline static bool isInstance(MapView *pointer)
	{
		return instances.find(pointer) != instances.end();
	}

	template <typename T>
	T *currentAction() const;

	inline bool isDragRemoving() const
	{
		if (!isDragging())
			return false;
		auto action = editorAction.as<MouseAction::RawItem>();
		auto modifiers = uiUtils->modifiers();

		return action && (modifiers & VME::ModifierKeys::Shift) && (modifiers & VME::ModifierKeys::Ctrl);
	}

private:
	/**
 		*	Keeps track of all MapView instances. This is necessary for QT to
		* validate MapView pointers in a QVariant.
		*
		* See:
		* QtUtil::associatedMapView
	*/
	static std::unordered_set<MapView *> instances;

	std::unique_ptr<UIUtils> uiUtils;

	friend class MapAction;

	std::shared_ptr<Map> _map;
	Viewport viewport;

	struct DragData
	{
		WorldPosition from, to;
	};
	std::optional<DragData> dragState;

	Camera camera;

	bool canRender = false;

	std::vector<MapView::Observer *> observers;

	Tile deepCopyTile(const Position position) const
	{
		return _map->getTile(position)->deepCopy();
	}

	MapHistory::Action newAction(MapHistory::ActionType actionType) const;
};

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
