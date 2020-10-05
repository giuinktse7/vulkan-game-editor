#include "map.h"

#include <iostream>

#include "tile_location.h"
#include "ecs/ecs.h"
#include "ecs/item_animation.h"
#include "debug.h"
#include "graphics/appearances.h"

#include <stack>

constexpr uint16_t DefaultWidth = 2048;
constexpr uint16_t DefaultHeight = 2048;

Map::Map()
    : root(quadtree::Node::NodeType::Root), width(DefaultWidth), height(DefaultHeight)
{
}

void Map::clear()
{
  root.clear();
}

void Map::moveSelectedItems(const Position source, const Position destination)
{
  TileLocation *from = getTileLocation(source);
  if (!from)
  {
    ABORT_PROGRAM("No tile to move.");
  }

  TileLocation &to = getOrCreateTileLocation(destination);

  if (from->tile()->allSelected())
  {
    std::unique_ptr<Tile> fromTile = from->dropTile();
    to.setTile(std::move(fromTile));
  }
  else
  {
    if (!to.hasTile())
    {
      to.setEmptyTile();
    }

    from->tile()->moveSelected(*to.tile());
  }
}

void Map::addItem(const Position position, uint16_t serverId)
{
  if (!Items::items.validItemType(serverId))
    return;

  auto &tile = getOrCreateTile(position);
  tile.addItem(Item(serverId));
}

MapRegion Map::getRegion(Position from, Position to) noexcept
{
  return MapRegion(*this, from, to);
}

Tile &Map::getOrCreateTile(const Position &pos)
{
  return getOrCreateTile(pos.x, pos.y, pos.z);
}

std::unique_ptr<Tile> Map::replaceTile(Tile &&tile)
{
  TileLocation &location = getOrCreateTileLocation(tile.position());

  return location.replaceTile(std::move(tile));
}

void Map::insertTile(Tile &&tile)
{
  TileLocation &location = getOrCreateTileLocation(tile.position());

  location.setTile(std::make_unique<Tile>(std::move(tile)));
}

void Map::moveTile(Position from, Position to)
{
  if (from == to)
  {
    VME_LOG("Warning: Attempted to move a tile to the same position it currently has.");
    return;
  }

  std::unique_ptr<Tile> tile = getTileLocation(from)->dropTile();
  getTileLocation(to)->setTile(std::move(tile));
}

void Map::removeTile(const Position pos)
{
  auto leaf = root.getLeafUnsafe(pos.x, pos.y);
  if (leaf)
  {
    Floor *floor = leaf->getFloor(pos.z);
    if (floor)
    {
      auto &loc = floor->getTileLocation(pos.x, pos.y);
      if (loc.hasTile())
      {
        loc.removeTile();
      }
    }
  }
}

std::unique_ptr<Tile> Map::dropTile(const Position pos)
{
  auto leaf = root.getLeafUnsafe(pos.x, pos.y);
  if (leaf)
  {
    Floor *floor = leaf->getFloor(pos.z);
    if (floor)
    {
      auto &loc = floor->getTileLocation(pos.x, pos.y);
      if (loc.hasTile())
      {
        return loc.dropTile();
      }
    }
  }

  return {};
}

bool Map::isTileEmpty(const Position pos) const
{
  Tile *tile = getTile(pos);
  return !tile || tile->isEmpty();
}

Tile *Map::getTile(const Position pos) const
{
  auto leaf = root.getLeafUnsafe(pos.x, pos.y);
  if (!leaf)
    return nullptr;

  Floor *floor = leaf->getFloor(pos.z);
  if (!floor)
    return nullptr;

  return floor->getTileLocation(pos.x, pos.y).tile();
}

const Item *Map::getTopItem(const Position pos) const
{
  Tile *tile = getTile(pos);

  if (!tile)
    return nullptr;

  return tile->getTopItem();
}

Tile &Map::getOrCreateTile(int x, int y, int z)
{
  DEBUG_ASSERT(root.isRoot(), "Only root nodes can create a tile.");
  auto &leaf = root.getLeafWithCreate(x, y);

  DEBUG_ASSERT(leaf.isLeaf(), "The node must be a leaf node.");

  Floor &floor = leaf.getOrCreateFloor(x, y, z);
  TileLocation &location = floor.getTileLocation(x, y);

  if (!location.tile())
  {
    location.setTile(std::make_unique<Tile>(location));
  }

  return *location.tile();
}

TileLocation *Map::getTileLocation(const Position &pos) const
{
  return getTileLocation(pos.x, pos.y, pos.z);
}

TileLocation *Map::getTileLocation(int x, int y, int z) const
{
  DEBUG_ASSERT(z >= 0 && z < MAP_LAYERS, "Z value '" + std::to_string(z) + "' is out of bounds.");
  quadtree::Node *leaf = root.getLeafUnsafe(x, y);
  if (leaf)
  {
    Floor *floor = leaf->getFloor(z);
    return &floor->getTileLocation(x, y);
  }

  return nullptr;
}

TileLocation &Map::getOrCreateTileLocation(const Position &pos)
{
  auto &leaf = root.getLeafWithCreate(pos.x, pos.y);
  TileLocation &location = leaf.getOrCreateTileLocation(pos);

  return location;
}

quadtree::Node *Map::getLeafUnsafe(int x, int y)
{
  return root.getLeafUnsafe(x, y);
}

MapIterator *MapIterator::nextFromLeaf()
{
  quadtree::Node *node = stack.top().node;
  DEBUG_ASSERT(node->isLeaf(), "The node must be a leaf node.");

  for (int z = this->floorIndex; z < MAP_LAYERS; ++z)
  {
    if (Floor *floor = node->getFloor(z))
    {
      for (uint32_t i = this->tileIndex; i < MAP_LAYERS; ++i)
      {
        TileLocation &location = floor->getTileLocation(i);
        if (location.hasTile() && !location.tile()->isEmpty())
        {
          this->value = &location;
          this->floorIndex = z;
          this->tileIndex = i + 1;

          return this;
        }
      }
      // Reset tile index before iterating next floor
      this->tileIndex = 0;
    }
  }

  return nullptr;
}

void MapIterator::emplace(quadtree::Node *node)
{
  stack.emplace(node);
}

MapIterator Map::begin()
{
  MapIterator iterator;
  iterator.emplace(&this->root);

  while (!iterator.stack.empty())
  {
    MapIterator::NodeIndex &current = iterator.stack.top();

    if (current.node->isLeaf())
    {
      auto next = iterator.nextFromLeaf();
      return next ? *next : end();
    }

    uint32_t size = static_cast<uint32_t>(current.node->nodes.size());
    for (uint32_t i = current.cursor; i < size; ++i)
    {
      if (auto &child = current.node->nodes[i])
      {
        current.cursor = i + 1;
        iterator.emplace(child.get());
        break;
      }
    }

    if (&iterator.stack.top().node == &current.node)
    {
      return end();
    }
  }

  return end();
}

void MapIterator::finish()
{
  this->value = nullptr;
  this->tileIndex = UINT32_MAX;
  this->floorIndex = UINT32_MAX;
}

MapIterator &MapIterator::operator++()
{
  while (!stack.empty())
  {
    MapIterator::NodeIndex &current = stack.top();

    if (current.node->isLeaf())
    {
      auto next = nextFromLeaf();
      if (!next)
      {
        stack.pop();
        continue;
      }

      return *next;
    }

    tileIndex = 0;
    floorIndex = 0;

    size_t size = current.node->nodes.size();
    // This node is finished
    if (current.cursor == size)
    {
      stack.pop();
      continue;
    }

    for (; current.cursor < size; ++current.cursor)
    {
      if (auto &child = current.node->nodes[current.cursor])
      {
        current.cursor += 1;
        emplace(child.get());
        break;
      }
    }
  }

  this->finish();
  return *this;
}

MapIterator Map::end()
{
  MapIterator iterator;
  return iterator.end();
}

MapIterator::MapIterator()
{
  // std::cout << "MapIterator()" << std::endl;
}

MapIterator::~MapIterator()
{
  // std::cout << "~MapIterator()" << std::endl;
}

TileLocation *MapIterator::operator*()
{
  return value;
}

TileLocation *MapIterator::operator->()
{
  return value;
}

MapVersion Map::getMapVersion()
{
  return mapVersion;
}

std::string &Map::getDescription()
{
  return description;
}

void Map::createItemAt(Position pos, uint16_t id)
{
  Item item(id);

  const SpriteInfo &spriteInfo = item.itemType->appearance->getSpriteInfo();
  if (spriteInfo.hasAnimation())
  {
    ecs::EntityId entityId = item.assignNewEntityId();
    g_ecs.addComponent(entityId, ItemAnimationComponent(spriteInfo.animation()));
  }

  getOrCreateTile(pos).addItem(std::move(item));
}

MapRegion::Iterator::Iterator(Map &map, Position from, Position to, bool isEnd)
    : map(map), from(from), to(to), isEnd(isEnd)
{
  if (!isEnd)
  {
    x1 = std::min(from.x, to.x);
    x2 = std::max(from.x, to.x);

    y1 = std::min(from.y, to.y);
    y2 = std::max(from.y, to.y);

    endZ = std::min(from.z, to.z);

    state.mapX = x1 & (~3);
    state.mapY = y1 & (~3);
    state.mapZ = std::max(from.z, to.z);

    nextChunk();
    updateValue();
  }
}

MapRegion::Iterator MapRegion::Iterator::operator++()
{
  ++state.chunk.y;
  updateValue();

  return *this;
}

MapRegion::Iterator MapRegion::Iterator::operator++(int)
{
  Iterator previous(*this);

  ++state.chunk.y;
  updateValue();

  return previous;
}

bool MapRegion::Iterator::operator==(const MapRegion::Iterator &rhs) const
{
  if (isEnd && rhs.isEnd)
    return true;

  return from == rhs.from && to == rhs.to && value == rhs.value;
}

void MapRegion::Iterator::nextChunk()
{
  for (; state.mapZ >= endZ; --state.mapZ)
  {
    for (; state.mapX <= x2; state.mapX += 4)
    {
      for (; state.mapY <= y2; state.mapY += 4)
      {
        quadtree::Node *node = map.getLeafUnsafe(state.mapX, state.mapY);
        if (node)
        {
          state.chunk.x = 0;
          state.chunk.y = 0;
          state.chunk.node = node;
          return;
        }
      }
      state.mapY = y1 & (~3);
    }
    state.mapX = x1 & (~3);
  }

  state.chunk.node = nullptr;
  isEnd = true;
}

void MapRegion::Iterator::updateValue()
{
  if (isEnd)
  {
    return;
  }

  for (int &x = state.chunk.x; x < 4; ++x)
  {
    if (state.mapX + x < x1 || state.mapX + x > x2)
    {
      continue;
    }
    for (int &y = state.chunk.y; y < 4; ++y)
    {
      if (state.mapY + y < y1 || state.mapY + y > y2)
      {
        continue;
      }
      Pointer tileLocation = state.chunk.node->getTile(state.mapX + x, state.mapY + y, state.mapZ);
      if (tileLocation)
      {
        value = tileLocation;
        // The function can return because the next TileLocation was found
        return;
      }
    }
    state.chunk.y = 0;
  }
  state.chunk.x = 0;

  state.mapY += 4;
  nextChunk();
  updateValue();
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>MapArea>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MapArea::MapArea(Position from, Position to) : from(from), to(to) {}

MapArea::iterator::iterator(Position from, Position to) : from(from), to(to), value(from)
{
  increasing.x = from.x <= to.x;
  increasing.y = from.y <= to.y;
  increasing.z = from.z <= to.z;
}

MapArea::iterator::iterator() {}

MapArea::iterator MapArea::iterator::end()
{
  auto it = MapArea::iterator();
  it.isEnd = true;

  return it;
}

MapArea::iterator MapArea::iterator::operator++()
{
  value.x += increasing.x ? 1 : -1;
  if (!inBoundsX())
  {
    value.x = from.x;
    value.y += increasing.y ? 1 : -1;
    if (!inBoundsY())
    {
      value.y = from.y;
      value.z += increasing.z ? 1 : -1;
      if (!inBoundsZ())
      {
        isEnd = true;
      }
    }
  }

  return *this;
}

MapArea::iterator MapArea::iterator::operator++(int)
{
  MapArea::iterator it = *this;
  ++(*this);

  return it;
}

bool MapArea::iterator::operator==(const MapArea::iterator &rhs) const
{
  return (isEnd && rhs.isEnd) ||
         (value == rhs.value && from == rhs.from && to == rhs.to);
}