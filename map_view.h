#pragma once

#include <memory>

#include <optional>
#include <set>

#include "map.h"
#include "camera.h"
#include "position.h"
#include "util.h"
#include "selection.h"

#include "action/action.h"

class MapAction;

struct Viewport
{
	Viewport();
	int width;
	int height;
	float zoom;

	float offsetX;
	float offsetY;
};

class MapView
{
public:
	MapView();

	EditorHistory history;

	Selection selection;

	Map *getMap() const
	{
		return map.get();
	}

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

	void panCamera(WorldPosition delta);
	void panCameraX(double delta);
	void panCameraY(double delta);

	void finishMoveSelection(const Position moveDestination);

	void addItem(const Position position, uint16_t id);

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

	void translateCamera(WorldPosition delta);
	void translateCameraZ(int z);
	void updateCamera(const ScreenPosition cursorPos);

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
		double value;
	};

	void panEvent(PanEvent event);

private:
	friend class MapAction;
	Viewport viewport;

	struct DragData
	{
		WorldPosition from, to;
	};
	std::optional<DragData> dragState;

	Camera camera;

	std::shared_ptr<Map> map;

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