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
#include "selection.h"

#include "action/action.h"

#include "gui/gui.h"

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

	MapView();
	MapView(std::shared_ptr<Map> map);

	bool showPreviewCursor = false;

	std::optional<Position> leftMouseDragPos;

	EditorHistory history;

	Selection selection;

	struct MouseAction
	{
		struct RawItem
		{
			uint16_t serverId;
		};
	};

	using MouseAction_t = std::variant<std::monostate, MouseAction::RawItem>;

	void setMouseAction(const MouseAction_t action)
	{
		_mouseAction = action;
	}

	Map *getMap() const
	{
		return map.get();
	}

	void mousePressEvent(VME::MouseButtons pressedButtons);
	void mouseMoveEvent(VME::MouseButtons pressedButtons);
	void mouseReleaseEvent(VME::MouseButtons pressedButtons);

	Tile *getTile(const Position pos) const;
	void insertTile(Tile &&tile);
	void removeTile(const Position pos);
	void selectTopItem(Tile &tile);
	void deselectTopItem(Tile &tile);
	void selectAll(Tile &tile);
	void clearSelection();
	bool hasSelectionMoveOrigin() const;
	bool isSelectionMoving() const;
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

	void zoomOut();
	void zoomIn();
	void resetZoom();

	void updateViewport();
	void setViewportSize(int width, int height);

	float getZoomFactor() const;

	const MapPosition worldToMapPos(WorldPosition worldPos) const;
	const Position screenToMapPos(ScreenPosition screenPos) const;
	const MapPosition windowToMapPos(ScreenPosition screenPos) const;
	const uint32_t windowToMapPos(int windowPos) const;
	const uint32_t mapToWorldPos(uint32_t mapPos) const;

	int getZ() const
	{
		return static_cast<uint32_t>(camera.floor);
	}

	void undo()
	{
		history.undoLast();
	}

	/*
		Synonym for getZ()
	*/
	int getFloor() const
	{
		return getZ();
	}

	inline uint32_t getX() const
	{
		return static_cast<uint32_t>(camera.position().x);
	}

	inline uint32_t getY() const
	{
		return static_cast<uint32_t>(camera.position().y);
	}

	const Viewport &getViewport() const
	{
		return viewport;
	}

	void deleteSelectedItems();

	util::Rectangle<int> getGameBoundingRect() const;

	void setDragStart(WorldPosition position);
	void setDragEnd(WorldPosition position);
	std::optional<std::pair<WorldPosition, WorldPosition>> getDragPoints() const
	{
		if (dragState.has_value())
		{
			std::pair<WorldPosition, WorldPosition> result;
			result.first = dragState.value().from;
			result.second = dragState.value().to;
			return result;
		}

		return {};
	}
	void endDragging();
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

	void mouseMoveEvent(ScreenPosition mousePos);
	void panEvent(PanEvent event);

	void rawItemSelectedEvent(uint16_t serverId);

	inline MouseAction_t mouseAction() const
	{
		return _mouseAction;
	}

	inline ScreenPosition mousePos() const
	{
		return _mousePos;
	}

	void setMousePos(const ScreenPosition pos);

	void addObserver(MapView::Observer *observer);
	void removeObserver(MapView::Observer *observer);
	void notifyObservers(MapView::Observer::ChangeType changeType) const;

private:
	friend class MapAction;

	std::shared_ptr<Map> map;
	Viewport viewport;

	MouseAction_t _mouseAction = std::monostate{};

	struct DragData
	{
		WorldPosition from, to;
	};
	std::optional<DragData> dragState;

	Camera camera;

	ScreenPosition _mousePos;

	std::vector<MapView::Observer *> observers;

	Tile deepCopyTile(const Position position) const
	{
		return map->getTile(position)->deepCopy();
	}

	/*
		Returns the old tile at the location of the tile.
	*/
	std::unique_ptr<Tile> setTileInternal(Tile &&tile);
	std::unique_ptr<Tile> removeTileInternal(const Position position);
	void removeSelectionInternal(Tile *tile);

	MapAction newAction(MapActionType actionType) const;
};

inline std::ostream &operator<<(std::ostream &os, const util::Rectangle<int> &rect)
{
	os << "{ x1=" << rect.x1 << ", y1=" << rect.y1 << ", x2=" << rect.x2 << ", y2=" << rect.y2 << "}";
	return os;
}