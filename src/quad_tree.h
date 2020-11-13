#pragma once

#include <array>
#include <memory>
#include <stdint.h>

#include "const.h"
#include "position.h"
#include "tile_location.h"

class MapIterator;
class Map;

class Floor
{
public:
	Floor(int x, int y, int z);
	~Floor() noexcept;

	Floor(const Floor &) = delete;
	Floor &operator=(const Floor &) = delete;

	TileLocation &getTileLocation(int x, int y);
	TileLocation &getTileLocation(uint32_t index);

private:
	// x, y locations
	TileLocation locations[MAP_TREE_CHILDREN_COUNT];
};

namespace quadtree
{
	class Node
	{
		enum class NodeType
		{
			Root,
			Node,
			Leaf
		};

	public:
		Node(NodeType nodeType);
		Node(NodeType nodeType, int level);
		~Node();

		void clear();

		int level = -1;

		Node(const Node &) = delete;
		Node &operator=(const Node &) = delete;

		// Get a leaf node. Creates the leaf node if it does not already exist.
		Node &getLeafWithCreate(int x, int y);
		Node *getLeafUnsafe(int x, int y) const;
		TileLocation *getTile(int x, int y, int z) const;

		Floor &getOrCreateFloor(Position pos);
		Floor &getOrCreateFloor(int x, int y, int z);
		Floor *floor(uint32_t z) const;

		TileLocation &getOrCreateTileLocation(Position pos);

		inline bool isLeaf() const noexcept;
		inline bool isRoot() const noexcept;

		friend class ::Map;
		friend class ::MapIterator;

	protected:
		NodeType nodeType = NodeType::Root;
		union
		{
			std::array<std::unique_ptr<Node>, MAP_TREE_CHILDREN_COUNT> nodes{};
			std::array<std::unique_ptr<Floor>, MAP_TREE_CHILDREN_COUNT> children;
		};
	};
}; // namespace quadtree

inline bool quadtree::Node::isLeaf() const noexcept
{
	return nodeType == NodeType::Leaf;
}

inline bool quadtree::Node::isRoot() const noexcept
{
	return nodeType == NodeType::Root;
}