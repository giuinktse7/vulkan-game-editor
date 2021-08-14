#include "map.h"

#include <iostream>

#include "debug.h"
#include "graphics/appearances.h"
#include "items.h"
#include "tile_location.h"

#include <stack>

constexpr uint16_t DefaultWidth = 2048;
constexpr uint16_t DefaultHeight = 2048;
constexpr uint16_t DefaultDepth = 16;

Map::Map()
    : Map(DefaultWidth, DefaultHeight, DefaultDepth) {}

Map::Map(std::string name, uint16_t width, uint16_t height)
    : _name(""), root(quadtree::Node::NodeType::Root), _size(DefaultWidth, DefaultHeight, DefaultDepth)
{
    DEBUG_ASSERT(util::powerOf2(_size.width()) && util::powerOf2(_size.height()) && util::powerOf2(_size.depth()), "Width & height must be powers of 2");
}

Map::Map(uint16_t width, uint16_t height)
    : Map("", width, height)
{
}

Map::Map(uint16_t width, uint16_t height, uint8_t depth)
    : _name(""), root(quadtree::Node::NodeType::Root), _size(DefaultWidth, DefaultHeight, DefaultDepth)
{
    DEBUG_ASSERT(util::powerOf2(_size.width()) && util::powerOf2(_size.height()) && util::powerOf2(_size.depth()), "Width & height must be powers of 2");
}

Map::Map(Map &&other) noexcept
    : _name(std::move(other._name)),
      _towns(std::move(other._towns)),
      mapVersion(std::move(other.mapVersion)),
      _description(std::move(other._description)),
      _spawnFilepath(std::move(other._spawnFilepath)),
      _houseFilepath(std::move(other._houseFilepath)),
      root(std::move(other.root)),
      _size(std::move(other._size))
{
}

Map &Map::operator=(Map &&other) noexcept
{
    _name = std::move(other._name);
    _towns = std::move(other._towns);
    mapVersion = std::move(other.mapVersion);
    _description = std::move(other._description);
    _spawnFilepath = std::move(other._spawnFilepath);
    _houseFilepath = std::move(other._houseFilepath);
    root = std::move(other.root);
    _size = std::move(other._size);

    return *this;
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

void Map::addItem(const Position position, uint32_t serverId)
{
    if (!Items::items.validItemType(serverId))
        return;

    auto &tile = getOrCreateTile(position);
    tile.addItem(Item(serverId));
}

MapRegion Map::getRegion(Position from, Position to) const noexcept
{
    return MapRegion(*this, from, to);
}

bool Map::contains(const Position &position) const noexcept
{
    return 0 <= position.x && position.x <= width() && 0 <= position.y && position.y <= height();
}

Tile &Map::getOrCreateTile(const Position &pos)
{
    DEBUG_ASSERT(root.isRoot(), "Only root nodes can create a tile.");
    auto &leaf = root.getLeafWithCreate(pos.x, pos.y);

    DEBUG_ASSERT(leaf.isLeaf(), "The node must be a leaf node.");

    auto &location = leaf.getOrCreateTileLocation(pos);

    if (!location.tile())
    {
        location.setTile(std::make_unique<Tile>(location));
    }

    return *location.tile();
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
        Floor *floor = leaf->floor(pos.z);
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

std::shared_ptr<Item> Map::dropItem(Tile *tile, Item *item)
{
    DEBUG_ASSERT(tile != nullptr && item != nullptr, "These may not be nullptr.");
    DEBUG_ASSERT(getTile(tile->position()) == tile, "The tile must be present in the map.");

    return tile->dropItem(item);
}

std::unique_ptr<Tile> Map::dropTile(const Position pos)
{
    auto location = getTileLocation(pos);
    if (location && location->hasTile())
    {
        return location->dropTile();
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

    Floor *floor = leaf->floor(pos.z);
    if (!floor)
        return nullptr;

    return floor->getTileLocation(pos.x, pos.y).tile();
}

bool Map::hasTile(const Position pos) const
{
    auto leaf = root.getLeafUnsafe(pos.x, pos.y);
    if (!leaf)
        return false;

    Floor *floor = leaf->floor(pos.z);

    return floor->getTileLocation(pos.x, pos.y).tile() != nullptr;
}

TileThing Map::getTopThing(const Position pos)
{
    return const_cast<const Map *>(this)->getTopThing(pos);
}

const TileThing Map::getTopThing(const Position pos) const
{
    Tile *tile = getTile(pos);

    if (!tile)
        return std::monostate{};

    return tile->getTopThing();
}

Item *Map::getTopItem(const Position pos)
{
    return const_cast<Item *>(const_cast<const Map *>(this)->getTopItem(pos));
}

const Item *Map::getTopItem(const Position pos) const
{
    Tile *tile = getTile(pos);

    if (!tile)
        return nullptr;

    return tile->getTopItem();
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
        Floor *floor = leaf->floor(z);
        if (floor)
        {
            return &floor->getTileLocation(x, y);
        }
    }

    return nullptr;
}

TileLocation &Map::getOrCreateTileLocation(const Position &pos)
{
    auto &leaf = root.getLeafWithCreate(pos.x, pos.y);
    TileLocation &location = leaf.getOrCreateTileLocation(pos);

    return location;
}

quadtree::Node *Map::getLeafUnsafe(int x, int y) const
{
    return root.getLeafUnsafe(x, y);
}

MapIterator *MapIterator::nextFromLeaf()
{
    quadtree::Node *node = stack.top().node;
    DEBUG_ASSERT(node->isLeaf(), "The node must be a leaf node.");

    for (int z = this->floorIndex; z < MAP_LAYERS; ++z)
    {
        if (Floor *floor = node->floor(z))
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

MapIterator Map::begin() const
{
    MapIterator iterator;
    iterator.emplace(&const_cast<Map *>(this)->root);

    while (!iterator.stack.empty())
    {
        auto &current = iterator.stack.top();

        if (current.node->isLeaf())
        {
            auto next = iterator.nextFromLeaf();
            return next ? *next : end();
        }

        uint32_t size = static_cast<uint32_t>(current.node->children.size());
        for (uint32_t i = current.cursor; i < size; ++i)
        {
            if (auto child = current.node->children.node(i))
            {
                current.cursor = i + 1;
                iterator.emplace(child);
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
        auto &current = stack.top();

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

        size_t size = current.node->children.size();
        // This node is finished
        if (current.cursor == size)
        {
            stack.pop();
            continue;
        }

        for (; current.cursor < size; ++current.cursor)
        {
            if (auto child = current.node->children.node(current.cursor))
            {
                current.cursor += 1;
                emplace(child);
                break;
            }
        }
    }

    this->finish();
    return *this;
}

MapIterator Map::end() const
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

const MapVersion &Map::getMapVersion() const
{
    return mapVersion;
}

const std::string &Map::description() const
{
    return _description;
}

const std::filesystem::path &Map::spawnFilepath() const
{
    return _spawnFilepath;
}
const std::filesystem::path &Map::houseFilepath() const
{
    return _houseFilepath;
}

void Map::setDescription(std::string description)
{
    _description = description;
}

void Map::setSpawnFilepath(std::filesystem::path path)
{
    _spawnFilepath = path;
}

void Map::setHouseFilepath(std::filesystem::path path)
{
    _houseFilepath = path;
}

void Map::createItemAt(Position pos, uint16_t id)
{
    Item item(id);

    getOrCreateTile(pos).addItem(std::move(item));
}

MapRegion::Iterator::Iterator(const Map &map, Position from, Position to, bool isEnd)
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

MapRegion::Iterator &MapRegion::Iterator::operator++()
{
    ++state.chunk.y;
    updateValue();

    return *this;
}

// MapRegion::Iterator MapRegion::Iterator::operator++(int)
// {

//   // Iterator previous(*this);

//   // ++state.chunk.y;
//   // updateValue();

//   // return previous;
// }

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
    while (!isEnd)
    {
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
    }
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>MapArea>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MapArea::MapArea(const Map &map, Position from, Position to)
{
    auto width = map.width();
    auto height = map.height();

    this->from = Position(std::clamp<Position::value_type>(from.x, 0, width), std::clamp<Position::value_type>(from.y, 0, height), from.z);
    this->to = Position(std::clamp<Position::value_type>(to.x, 0, width), std::clamp<Position::value_type>(to.y, 0, height), to.z);

    // Skip iterating if there can't be any positions in [from, to]
    if (!map.contains(from) && !map.contains(to) && this->from.x == this->to.x && this->from.y == this->to.y)
    {
        empty = true;
    }
}

MapArea::iterator::iterator(Position from, Position to)
    : from(from), to(to), value(from)
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

MapArea::iterator &MapArea::iterator::operator++()
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

void Map::addTown(Town &&town)
{
    _towns.emplace(town.id(), std::move(town));
}

const Town *Map::getTown(const std::string &name) const
{
    auto found = std::find_if(_towns.begin(), _towns.end(), [&name](const std::pair<uint32_t, Town> &pair) {
        return pair.second.name() == name;
    });

    return found != _towns.end() ? &found->second : nullptr;
}

Town *Map::getTown(const std::string &name)
{
    auto found = std::find_if(_towns.begin(), _towns.end(), [&name](const std::pair<uint32_t, Town> &pair) {
        return pair.second.name() == name;
    });

    return found != _towns.end() ? &found.value() : nullptr;
}

Town *Map::getTown(uint32_t id)
{
    auto found = _towns.find(id);
    return found != _towns.end() ? &found.value() : nullptr;
}

const Town *Map::getTown(uint32_t id) const
{
    auto found = _towns.find(id);
    return found != _towns.end() ? &found.value() : nullptr;
}
