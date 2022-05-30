#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "tileset.h"

struct ItemPalettes;

class ItemPalette
{
  public:
    // Non-copyable
    ItemPalette(const ItemPalette &other) = delete;
    ItemPalette operator&=(const ItemPalette &other) = delete;

    ItemPalette(ItemPalette &&other) = default;
    ItemPalette &operator=(ItemPalette &&other) = default;

    Tileset &addTileset(Tileset &&tileset);
    Tileset &addTileset(std::unique_ptr<Tileset> tileset);

    const std::string &name() const noexcept;
    const std::string &id() const noexcept;

    bool empty() const noexcept;

    Tileset *tileset(size_t index);
    Tileset *getTileset(const std::string &id);
    const std::vector<std::unique_ptr<Tileset>> &tilesets();

  private:
    friend struct ItemPalettes;

    ItemPalette(const std::string &id, const std::string &name);

    std::unordered_map<std::string, size_t> tilesetIndexMap;
    std::vector<std::unique_ptr<Tileset>> _tilesets;
    std::string _id;
    std::string _name;
};

struct ItemPalettes
{
    static ItemPalette &createPalette(const std::string &id, const std::string &name);
    static ItemPalette *getById(const std::string &id);
    static ItemPalette *getOrCreateById(const std::string &id);

    static std::unordered_map<std::string, ItemPalette> &itemPalettes();

  private:
    static std::unordered_map<std::string, ItemPalette> _itemPalettes;

    static bool contains(const std::string &id);
};

template <typename K, typename T>
class MapValueIterator
{
  public:
    using ValueType = T;
    using Reference = const ValueType &;
    using Pointer = T *;
    using IteratorCategory = std::forward_iterator_tag;

    MapValueIterator(const std::unordered_map<K, T> &map)
        : map(map)
    {
        it = map.begin();
    }

    MapValueIterator<T, K> end()
    {
        return it == map.end();
    }

    MapValueIterator &operator++()
    {
        ++it;
        return *this;
    }
    MapValueIterator operator++(int junk)
    {
        MapValueIterator iter = *this;
        ++(*this);

        return iter;
    }

    const ValueType &operator*() const
    {
        return it->second;
    }
    const ValueType &operator->() const
    {
        return it->second;
    }

    bool operator==(const MapValueIterator &rhs) const
    {
        bool end1 = it == map.end();
        bool end2 = rhs.it == rhs.map.end();
        return !(end1 ^ end2) && ((end1 && end2) || (it->second == rhs.it->second));
    }

    bool operator!=(const MapValueIterator &rhs) const
    {
        return !(*this == rhs);
    }

  private:
    const std::unordered_map<K, T> &map;
    typename std::unordered_map<K, T>::const_iterator it;
};