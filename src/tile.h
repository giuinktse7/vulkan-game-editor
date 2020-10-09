#pragma once

#include <memory>
#include <optional>
#include <algorithm>

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

	/**
	 * @return The amount of removed items
	 * */
	template <typename UnaryPredicate>
	inline uint16_t removeItemsIf(UnaryPredicate &&predicate);

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

	const std::vector<Item> &items() const noexcept
	{
		return _items;
	}

	const size_t itemCount() const noexcept
	{
		return _items.size();
	}

	/*
		Counts all entities (items, creature, spawn, waypoint, etc.).
	*/
	size_t getEntityCount();

	uint16_t getMapFlags() const noexcept;
	uint16_t getStatFlags() const noexcept;

	void setLocation(TileLocation &location);

	void initEntities();
	void destroyEntities();

	inline Position position() const noexcept
	{
		return _position;
	}

	inline Position::value_type x() const noexcept
	{
		return _position.x;
	}

	inline Position::value_type y() const noexcept
	{
		return _position.y;
	}

	inline Position::value_type z() const noexcept
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

	void replaceGround(Item &&ground);
	void replaceItem(uint16_t index, Item &&item);
};

inline uint16_t Tile::getMapFlags() const noexcept
{
	return mapflags;
}
inline uint16_t Tile::getStatFlags() const noexcept
{
	return statflags;
}

template <typename UnaryPredicate>
inline uint16_t Tile::removeItemsIf(UnaryPredicate &&predicate)
{
	uint16_t removedItems = 0;
	if (_ground && std::forward<UnaryPredicate>(predicate)(*(_ground.get())))
	{
		removeGround();
		++removedItems;
	}

	if (!_items.empty())
	{
		auto removed = std::remove_if(
				_items.begin(),
				_items.end(),
				[this, &predicate, &removedItems](const Item &item) {
					bool remove = std::forward<UnaryPredicate>(predicate)(item);

					if (remove)
					{
						++removedItems;
						if (item.selected)
							--this->selectionCount;
					}

					return remove;
				});
		_items.erase(removed, _items.end());
	}

	return removedItems;
}