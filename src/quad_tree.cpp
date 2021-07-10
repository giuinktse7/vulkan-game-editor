#include "quad_tree.h"

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "debug.h"

using namespace quadtree;
using namespace std;

// uint32_t has 32 bits: +- get 16 bits each. 4 least sig.
// is used within a chunk. This gives (16 - 4) / 2 = 6 levels in the tree.
constexpr uint32_t QuadTreeDepth = 6;

// The implementation assumes a map in the range [-65535, 65535]

Node::Node(Node::NodeType nodeType)
    : nodeType(nodeType), children(nodeType)
{
    // cout << "Construct node" << endl;
}

Node::Node(Node &&other) noexcept
    : nodeType(std::move(other.nodeType)),
      children(std::move(other.children)) {}

Node &Node::operator=(Node &&other) noexcept
{
    nodeType = std::move(other.nodeType);
    children = std::move(other.children);

    return *this;
}

void Node::clear()
{
    DEBUG_ASSERT(isRoot(), "Only a root can be cleared.");

    children.reset();
}

Floor &Node::getOrCreateFloor(const Position &pos)
{
    DEBUG_ASSERT(isLeaf(), "Only leaf nodes can create a floor.");

    return children.getOrCreateFloor(pos);
}

TileLocation &Node::getOrCreateTileLocation(const Position &pos)
{
    return getOrCreateFloor(pos).getTileLocation(pos.x, pos.y);
}

TileLocation *Node::getTile(int x, int y, int z) const
{
    DEBUG_ASSERT(isLeaf(), "Only leaves can contain tiles.");

    Floor *f = floor(z);
    if (!f)
        return nullptr;

    return &f->getTileLocation(x, y);
}

Floor *Node::floor(uint32_t z) const
{
    DEBUG_ASSERT(isLeaf(), "Only leaves contain floors.");
    return children.floor(z);
}

Node *Node::getLeafUnsafe(int x, int y) const
{
    Node *node = const_cast<Node *>(this);

    uint32_t currentX = x;
    uint32_t currentY = y;

    while (!node->isLeaf())
    {
        uint32_t index = ((currentX & 0xC000) >> 14) | ((currentY & 0xC000) >> 12);

        std::unique_ptr<Node> &child = node->children.nodePtr(index);
        if (!child)
        {
            return nullptr;
        }

        node = child.get();
        currentX <<= 2;
        currentY <<= 2;
    }

    return node;
}

Node &Node::getLeafWithCreate(int x, int y)
{
    Node *node = this;
    uint32_t currentX = x;
    uint32_t currentY = y;

    int level = static_cast<int>(QuadTreeDepth);

    while (level > 0)
    {
        /*  The index is given by the bytes YYXX.
        XX is given by the two MSB in currentX, and YY by the two MSB in currentY.
    */
        uint32_t index = ((currentX & 0xC000) >> 14) | ((currentY & 0xC000) >> 12);

        // The NodeTypeCreationMapping array avoids a branch based on the node type
        node = node->children.getOrCreateNode(index, NodeType::Node);
        currentX <<= 2;
        currentY <<= 2;
        --level;
    }

    uint32_t index = ((currentX & 0xC000) >> 14) | ((currentY & 0xC000) >> 12);

    node = node->children.getOrCreateNode(index, NodeType::Leaf);

    return *node;
}

Floor::Floor(int x, int y, int z)
{
    // cout << "Floor()" << endl;
    // Since the map is chunked into 4x4, the first two bytes do not matter here
    // for x and y
    x &= ~3;
    y &= ~3;

    /* The tiles are stored as (01 means x = 0, y = 1):
      00, 01, 02, 03,
      10, 11, 12, 13,
      20, 21, 22, 23,
      30, 31, 32, 33,
  */
    for (int i = 0; i < MAP_TREE_CHILDREN_COUNT; ++i)
    {
        locations[i]._position.x = x + (i >> 2);
        locations[i]._position.y = y + (i & 3);
        locations[i]._position.z = z;
    }
}

Floor::Floor(Floor &&other) noexcept
    : locations(std::move(other.locations))
{
}

Floor &Floor::operator=(Floor &&other) noexcept
{
    locations = std::move(other.locations);

    return *this;
}

TileLocation &Floor::getTileLocation(int x, int y)
{
    return locations[(x & 3) * 4 + (y & 3)];
}

TileLocation &Floor::getTileLocation(uint32_t index)
{
    DEBUG_ASSERT(index < MAP_LAYERS, "Index '" + std::to_string(index) + "' is larger than MAP_LAYERS (=" + std::to_string(MAP_LAYERS) + ").");

    return locations[index];
}

/*
  The Children code is quite intricate and uses low-level concepts like placement-new.
  It is easy to shoot yourself in the foot here. Only modify if you know what you are doing.
*/
//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>Children>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>

Node::Children::Children(Node::NodeType type)
    : _type(type)
{
    construct();
}

Node::Children::Children(Children &&other) noexcept
    : _type(other._type)
{
    construct();

    if (_type != NodeType::Leaf)
    {
        for (int i = 0; i < Amount; ++i)
        {
            nodes[i] = std::move(other.nodes[i]);
        }
    }
    else
    {
        for (int i = 0; i < Amount; ++i)
        {
            floors[i] = std::move(other.floors[i]);
        }
    }
}

Node::Children &Node::Children::operator=(Children &&other) noexcept
{
    if (_type != other._type)
    {
        destruct();
    }
    _type = other._type;
    construct();

    if (_type != NodeType::Leaf)
    {
        for (int i = 0; i < Amount; ++i)
        {
            nodes[i] = std::move(other.nodes[i]);
        }
    }
    else
    {
        for (int i = 0; i < Amount; ++i)
        {
            floors[i] = std::move(other.floors[i]);
        }
    }

    return *this;
}

Node::Children::~Children()
{
    destruct();
}

void Node::Children::construct()
{
    if (_type != NodeType::Leaf)
    {
        for (int i = 0; i < Amount; ++i)
        {
            new (&nodes[i]) std::unique_ptr<Node>;
        }
    }
    else
    {
        for (int i = 0; i < Amount; ++i)
        {
            new (&floors[i]) std::unique_ptr<Floor>;
        }
    }
}

void Node::Children::destruct()
{
    if (_type != NodeType::Leaf)
    {
        for (int i = 0; i < Amount; ++i)
        {
            nodes[i].reset();
        }
    }
    else
    {
        for (int i = 0; i < Amount; ++i)
        {
            floors[i].reset();
        }
    }
}

void Node::Children::reset()
{
    destruct();
}

void Node::Children::setNode(size_t index, std::unique_ptr<Node> &&value)
{
    if (_type != NodeType::Leaf)
    {
        nodes[index] = std::move(value);
    }
}

void Node::Children::setFloor(size_t index, std::unique_ptr<Floor> &&value)
{
    if (_type == NodeType::Leaf)
    {
        floors[index] = std::move(value);
    }
}

std::unique_ptr<Node> &Node::Children::nodePtr(size_t index)
{
    DEBUG_ASSERT(index < Amount && _type != NodeType::Leaf, "Bad union access.");

    return nodes[index];
}

Node *Node::Children::getOrCreateNode(size_t index, NodeType type)
{
    auto &node = nodes[index];
    if (!node)
        node = std::make_unique<Node>(type);

    return node.get();
}

std::unique_ptr<Floor> &Node::Children::floorPtr(size_t index)
{
    DEBUG_ASSERT(index < Amount && _type == NodeType::Leaf, "Bad union access.");

    return floors[index];
}

Floor &Node::Children::getOrCreateFloor(const Position &pos)
{
    auto &floor = floors[pos.z];
    if (!floor)
        floor = std::make_unique<Floor>(pos.x, pos.y, pos.z);

    return *floor;
}

Node *Node::Children::node(size_t index) const
{
    if (index >= Amount)
        return nullptr;

    return _type != NodeType::Leaf ? nodes[index].get() : nullptr;
}

Floor *Node::Children::floor(size_t index) const
{
    if (index >= Amount)
        return nullptr;

    return _type == NodeType::Leaf ? floors[index].get() : nullptr;
}

size_t Node::Children::size() const noexcept
{
    return Amount;
}