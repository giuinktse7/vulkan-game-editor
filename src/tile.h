#pragma once

#include <memory>
#include <optional>

#include "tile_location.h"
#include "item.h"

class MapView;
class MapAction;
class TileLocation;

class Tile
{
public:
	Tile(Position position);

	Tile(TileLocation &location);
	~Tile();

	Tile(const Tile &) = delete;
	Tile &operator=(const Tile &) = delete;
	Tile(Tile &&other) noexcept;
	Tile &operator=(Tile &&other) noexcept;

	Tile deepCopy() const;

	inline bool itemSelected(uint16_t itemIndex) const
	{
		return _items.at(itemIndex).selected;
	}
	bool hasSelection() const;
	bool topItemSelected() const;
	bool allSelected() const;

	Item *getTopItem() const;
	bool hasTopItem() const;
	Item *ground() const;
	inline bool hasGround() const
	{
		return static_cast<bool>(_ground);
	}

	void addItem(Item &&item);
	void removeItem(size_t index);
	Item dropItem(size_t index);
	void removeGround();
	std::unique_ptr<Item> dropGround();
	void setGround(std::unique_ptr<Item> ground);
	void moveItems(Tile &other);
	void moveSelected(Tile &other);

	/*
		Deselect entire tile
	*/
	void deselectAll();
	void deselectTopItem();
	void selectTopItem();
	void selectItemAtIndex(size_t index);
	void deselectItemAtIndex(size_t index);
	void setItemSelected(size_t itemIndex, bool selected);
	void selectGround();
	void deselectGround();
	void setGroundSelected(bool selected);
	void selectAll();

	bool isEmpty() const;

	int getTopElevation() const;

	const std::vector<Item> &items() const
	{
		return _items;
	}

	const size_t itemCount() const
	{
		return _items.size();
	}

	/*
		Counts all entities (items, creature, spawn, waypoint, etc.).
	*/
	size_t getEntityCount();

	uint16_t getMapFlags() const;
	uint16_t getStatFlags() const;

	void setLocation(TileLocation &location);

	void initEntities();
	void destroyEntities();

	inline Position position() const
	{
		return _position;
	}

	inline long x() const
	{
		return _position.x;
	}

	inline long y() const
	{
		return _position.y;
	}

	inline long z() const
	{
		return _position.z;
	}

private:
	friend class MapView;
	friend class MapAction;

	Position _position;
	std::unique_ptr<Item> _ground;
	std::vector<Item> _items;

	size_t selectionCount;

	// This structure makes it possible to access all flags, or map/stat flags separately.
	union
	{
		struct
		{
			uint16_t mapflags;
			uint16_t statflags;
		};
		uint32_t flags;
	};
};

inline uint16_t Tile::getMapFlags() const
{
	return mapflags;
}
inline uint16_t Tile::getStatFlags() const
{
	return statflags;
}