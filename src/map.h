#pragma once

#include <memory>
#include <unordered_map>
#include <iostream>
#include <string>
#include <stack>
#include <optional>

#include "debug.h"

#include "item.h"
#include "tile.h"
#include "tile_location.h"
#include "quad_tree.h"
#include "position.h"
#include "util.h"

#include "town.h"

#include "version.h"

class MapView;
class MapRegion;

class Map
{
public:
	Map();

	MapIterator begin();
	MapIterator end();

	MapRegion getRegion(Position from, Position to);

	TileLocation *getTileLocation(int x, int y, int z) const;
	TileLocation *getTileLocation(const Position &pos) const;
	Tile *getTile(const Position pos) const;

	bool isTileEmpty(const Position pos) const;

	MapVersion getMapVersion();
	std::string &getDescription();

	uint16_t getWidth() const;
	uint16_t getHeight() const;

	Towns &getTowns()
	{
		return towns;
	}

	inline const std::string &name() const
	{
		return _name;
	}

	void setName(std::string name)
	{
		_name = name;
	}

	/*
		Clear the map.
	*/
	void clear();

	quadtree::Node *getLeafUnsafe(int x, int y);

private:
	std::string _name;
	friend class MapView;
	Towns towns;
	MapVersion mapVersion;
	std::string description;

	quadtree::Node root;

	uint16_t width, height;

	/*
		Replace the tile at the given tile's location. Returns the old tile if one
		was present.
	*/
	std::unique_ptr<Tile> replaceTile(Tile &&tile);
	void insertTile(Tile &&tile);
	Tile &getOrCreateTile(int x, int y, int z);
	Tile &getOrCreateTile(const Position &pos);
	TileLocation &getOrCreateTileLocation(const Position &pos);
	void removeTile(const Position pos);

	void moveSelectedItems(const Position source, const Position destination);
	/*
		Remove and release ownership of the tile
	*/
	std::unique_ptr<Tile> dropTile(const Position pos);
	void createItemAt(Position pos, uint16_t id);
};

inline uint16_t Map::getWidth() const
{
	return width;
}
inline uint16_t Map::getHeight() const
{
	return height;
}

// Iterator for a map region
class MapRegion
{
public:
	MapRegion(Map &map, Position from, Position to)
			: map(map), from(from), to(to) {}

	class Iterator
	{
	public:
		using ValueType = TileLocation;
		using Reference = TileLocation &;
		using Pointer = TileLocation *;
		using IteratorCategory = std::forward_iterator_tag;

		Iterator(Map &map, Position from, Position to, bool isEnd = false);

		Iterator operator++();
		Iterator operator++(int junk);

		Reference operator*() { return *value; }
		Pointer operator->() { return value; }

		bool operator==(const Iterator &rhs) const;
		bool operator!=(const Iterator &rhs) const { return !(*this == rhs); }

	private:
		Map &map;
		Position from;
		Position to;

		int x1 = 0;
		int x2 = 0;
		int y1 = 0;
		int y2 = 0;
		int endZ = 0;

		struct State
		{
			int mapX = 0;
			int mapY = 0;
			int mapZ = 0;
			struct
			{
				int x = 0;
				int y = 0;
				quadtree::Node *node;
			} chunk;
		} state;

		Pointer value;

		bool isEnd;

		// For postfix operator
		Iterator(Map &map, const Iterator &iterator);

		void nextChunk();
		void updateValue();
		void reachedEnd();
	};

	Iterator begin()
	{
		return Iterator(map, from, to);
	}

	Iterator end()
	{
		return Iterator(map, from, to, true);
	}

private:
	Map &map;

	Position from;
	Position to;
};

class MapIterator
{
public:
	MapIterator();
	~MapIterator();

	MapIterator begin() const
	{
		return *this;
	}

	MapIterator *nextFromLeaf();

	MapIterator end()
	{
		MapIterator iterator;
		iterator.finish();
		return iterator;
	}

	// Mark the iterator as finished
	void finish();

	void emplace(quadtree::Node *node);

	TileLocation *operator*();
	TileLocation *operator->();
	MapIterator &operator++();

	bool operator==(const MapIterator &other) const
	{
		return other.floorIndex == floorIndex &&
					 other.tileIndex == tileIndex &&
					 other.value == value;
	}

	bool operator!=(const MapIterator &other) const
	{
		return !(other == *this);
	}

	struct NodeIndex
	{
		uint32_t cursor = 0;
		quadtree::Node *node;

		NodeIndex(quadtree::Node *node) : cursor(0), node(node) {}

		// bool operator==(const NodeIndex &other)
		// {
		// 	return other.cursor == cursor && &other.node == &node;
		// }
		// bool operator==(NodeIndex &other)
		// {
		// 	return other.cursor == cursor && &other.node == &node;
		// }
	};

	friend class Map;

private:
	std::stack<NodeIndex> stack{};
	uint32_t tileIndex = 0;
	uint32_t floorIndex = 0;
	TileLocation *value = nullptr;
};