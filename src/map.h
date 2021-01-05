#pragma once

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>

#include "debug.h"

#include "item.h"
#include "position.h"
#include "quad_tree.h"
#include "tile.h"
#include "tile_location.h"
#include "util.h"

#include "town.h"

#include "version.h"

class MapView;
class MapRegion;

namespace MapHistory
{
    class ChangeItem;
}

class Map
{
  public:
    Map();
    Map(uint16_t width, uint16_t height);
    Map(uint16_t width, uint16_t height, uint8_t depth);

    Map(Map &&other) noexcept;
    Map &operator=(Map &&other) noexcept;

    MapIterator begin() const;
    MapIterator end() const;

    Tile &getOrCreateTile(const Position &pos);
    TileLocation &getOrCreateTileLocation(const Position &pos);

    MapRegion getRegion(Position from, Position to) const noexcept;

    TileLocation *getTileLocation(int x, int y, int z) const;
    TileLocation *getTileLocation(const Position &pos) const;
    bool hasTile(const Position pos) const;
    Tile *getTile(const Position pos) const;
    const Item *getTopItem(const Position pos) const;
    Item *getTopItem(const Position pos);

    /*
		Remove and release ownership of the tile
	*/
    std::unique_ptr<Tile> dropTile(const Position pos);

    bool isTileEmpty(const Position pos) const;

    void addItem(const Position position, uint32_t serverId);

    void insertTile(Tile &&tile);
    /*
		Moves the tile to pos. If pos already contained a tile, that tile is destroyed.
	*/
    void moveTile(Position from, Position to);

    const MapVersion &getMapVersion() const;
    const std::string &description() const;
    const std::filesystem::path &spawnFilepath() const;
    const std::filesystem::path &houseFilepath() const;

    void setDescription(std::string description);
    void setSpawnFilepath(std::filesystem::path path);
    void setHouseFilepath(std::filesystem::path path);

    const util::Volume<uint16_t, uint16_t, uint8_t> size() const noexcept;
    uint16_t width() const noexcept;
    uint16_t height() const noexcept;
    uint8_t depth() const noexcept;

    inline const vme_unordered_map<uint32_t, Town> &towns() const noexcept;
    void addTown(Town &&town);
    const Town *getTown(const std::string &name) const;
    Town *getTown(const std::string &name);
    const Town *getTown(uint32_t id) const;
    Town *getTown(uint32_t id);

    inline const std::string &name() const noexcept;

    void setName(std::string name);

    /*
		Clear the map.
	*/
    void clear();

    quadtree::Node *getLeafUnsafe(int x, int y) const;

  private:
    friend class MapView;
    friend class MapHistory::ChangeItem;
    std::string _name;
    vme_unordered_map<uint32_t, Town> _towns;
    MapVersion mapVersion;
    std::string _description;

    std::filesystem::path _spawnFilepath;
    std::filesystem::path _houseFilepath;

    quadtree::Node root;

    util::Volume<uint16_t, uint16_t, uint8_t> _size;

    /*
		Replace the tile at the given tile's location. Returns the old tile if one
		was present.
	*/
    std::unique_ptr<Tile> replaceTile(Tile &&tile);
    std::unique_ptr<Tile> setOrReplaceTile(Tile &&tile);

    void removeTile(const Position pos);

    void moveSelectedItems(const Position source, const Position destination);

    /*
		Remove and release ownership of the item
	*/
    Item dropItem(Tile *tile, Item *item);

    void createItemAt(Position pos, uint16_t id);
};

inline const util::Volume<uint16_t, uint16_t, uint8_t> Map::size() const noexcept
{
    return _size;
}

inline uint16_t Map::width() const noexcept
{
    return _size.width();
}

inline uint16_t Map::height() const noexcept
{
    return _size.height();
}

inline uint8_t Map::depth() const noexcept
{
    return _size.depth();
}

inline const vme_unordered_map<uint32_t, Town> &Map::towns() const noexcept
{
    return _towns;
}

inline const std::string &Map::name() const noexcept
{
    return _name;
}

inline void Map::setName(std::string name)
{
    _name = name;
}

// Iterator for a map region
class MapRegion
{
  public:
    MapRegion(const Map &map, Position from, Position to) noexcept
        : map(map), from(from), to(to) {}

    class Iterator
    {
      public:
        using ValueType = TileLocation;
        using Reference = TileLocation &;
        using Pointer = TileLocation *;
        using IteratorCategory = std::forward_iterator_tag;

        Iterator(const Map &map, Position from, Position to, bool isEnd = false);
        Iterator(const Iterator &other) = delete;
        Iterator &operator=(const Iterator &other) = delete;

        Iterator &operator++();
        // Iterator operator++(int junk);

        Reference operator*()
        {
            return *value;
        }
        Pointer operator->()
        {
            return value;
        }

        bool operator==(const Iterator &rhs) const;
        bool operator!=(const Iterator &rhs) const
        {
            return !(*this == rhs);
        }

      private:
        const Map &map;
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

        void nextChunk();
        void updateValue();
    };

    Iterator begin()
    {
        return Iterator(map, from, to, false);
    }

    Iterator end()
    {
        return Iterator(map, from, to, true);
    }

  private:
    const Map &map;

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

    bool operator==(const MapIterator &other) const noexcept
    {
        return other.floorIndex == floorIndex &&
               other.tileIndex == tileIndex &&
               other.value == value;
    }

    bool operator!=(const MapIterator &other) const noexcept
    {
        return !(other == *this);
    }

    struct NodeIndex
    {
        uint32_t cursor = 0;
        quadtree::Node *node;

        NodeIndex(quadtree::Node *node)
            : cursor(0), node(node) {}

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

class MapArea
{
  public:
    MapArea(Position from, Position to);

    Position from;
    Position to;

    class iterator
    {
      public:
        using ValueType = Position;
        using Reference = Position &;
        using Pointer = Position *;
        using IteratorCategory = std::forward_iterator_tag;

        iterator(Position from, Position to);
        static iterator end();

        iterator &operator++();
        iterator operator++(int junk);

        Reference operator*()
        {
            return value;
        }
        Pointer operator->()
        {
            return &value;
        }

        bool operator==(const iterator &rhs) const;
        bool operator!=(const iterator &rhs) const
        {
            return !(*this == rhs);
        }

      private:
        Position from;
        Position to;

        Position value;

        inline bool inBoundsX() const noexcept
        {
            return increasing.x ? value.x <= to.x : value.x >= to.x;
        }

        inline bool inBoundsY() const noexcept
        {
            return increasing.y ? value.y <= to.y : value.y >= to.y;
        }

        inline bool inBoundsZ() const noexcept
        {
            return increasing.z ? value.z <= to.z : value.z >= to.z;
        }

        /* Information about whether x, y, and z are increasing from the start position to the end position */
        struct
        {
            bool x;
            bool y;
            bool z;
        } increasing;

        bool isEnd = false;

      private:
        iterator();
    };

    iterator begin()
    {
        return iterator(from, to);
    }

    iterator end()
    {
        return iterator::end();
    }
};