#pragma once

#include <algorithm>
#include <memory>
#include <optional>

#include "creature.h"
#include "item.h"
#include "tile_location.h"

class MapView;
class TileLocation;

class Tile
{
public:
	Tile(Position position);

	Tile(TileLocation &location);
	// ~Tile();

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
	inline size_t selectionCount() const noexcept;
	Item *firstSelectedItem();
	const Item *firstSelectedItem() const;

	Item *getTopItem() const;
	bool hasTopItem() const;
	inline Item *ground() const noexcept;
	inline bool hasGround() const noexcept;

	void addItem(Item &&item);
	void removeItem(size_t index);
	void removeItem(Item *item);
	void removeItem(std::function<bool(const Item &)> predicate);
	Item dropItem(size_t index);
	Item dropItem(Item *item);
	void removeGround();
	std::unique_ptr<Item> dropGround();
	void setGround(std::unique_ptr<Item> ground);
	void moveItems(Tile &other);
	void moveSelected(Tile &other);

	const std::vector<Item>::const_iterator findItem(std::function<bool(const Item &)> predicate) const;

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

	inline uint16_t mapFlags() const noexcept;
	inline uint16_t statFlags() const noexcept;
	inline uint32_t flags() const noexcept;

	void setFlags(uint32_t flags);

	void setLocation(TileLocation &location);
	void setCreature(Creature &&creature);
	void setCreature(std::unique_ptr<Creature> &&creature);
	std::unique_ptr<Creature> dropCreature();
	inline Creature *creature() const noexcept;
	inline bool hasCreature() const noexcept;

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

	void replaceGround(Item &&ground);
	void replaceItem(size_t index, Item &&item);

	std::vector<Item> _items;
	Position _position;
	std::unique_ptr<Item> _ground;
	std::unique_ptr<Creature> _creature;

	// This structure makes it possible to access all flags, or map/stat flags separately.
	union
	{
		struct
		{
			uint16_t _mapflags;
			uint16_t _statflags;
		};
		uint32_t _flags;
	};

	uint16_t _selectionCount;
};

inline uint16_t Tile::mapFlags() const noexcept
{
	return _mapflags;
}

inline uint16_t Tile::statFlags() const noexcept
{
	return _statflags;
}

inline uint32_t Tile::flags() const noexcept
{
	return _flags;
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
							--this->_selectionCount;
					}

					return remove;
				});
		_items.erase(removed, _items.end());
	}

	return removedItems;
}

inline bool Tile::hasGround() const noexcept
{
	return static_cast<bool>(_ground);
}

Item *Tile::ground() const noexcept
{
	return _ground.get();
}

inline size_t Tile::selectionCount() const noexcept
{
	return static_cast<size_t>(_selectionCount);
}

inline bool operator==(const Tile &lhs, const Tile &rhs)
{
	const Item *g1 = lhs.ground();
	const Item *g2 = rhs.ground();
	return lhs.flags() == rhs.flags() && lhs.items() == rhs.items() && (!(g1 || g2) || (g1 && g2 && (*g1 == *g2)));
}

inline bool operator!=(const Tile &lhs, const Tile &rhs)
{
	return !(lhs == rhs);
}

inline Creature *Tile::creature() const noexcept
{
	return _creature ? _creature.get() : nullptr;
}

inline bool Tile::hasCreature() const noexcept
{
	return static_cast<bool>(_creature);
}
