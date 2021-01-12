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
    void addTileset(Tileset &&tileset);
    Tileset *createTileset(const std::string &name);
    const std::string &name() const noexcept;

    bool empty() const noexcept;

    Tileset *tileset(size_t index);
    Tileset *tileset(const std::string &name);
    const std::vector<std::unique_ptr<Tileset>> &tilesets();

  private:
    friend struct ItemPalettes;

    ItemPalette(std::string name);

    std::unordered_map<std::string, size_t> tilesetIndexMap;
    std::vector<std::unique_ptr<Tileset>> _tilesets;
    std::string _name;
};

struct ItemPalettes
{
    static ItemPalette &createPalette(const std::string &name);
    static ItemPalette *getByName(const std::string &name);

    // static std::unordered_map<std::string, ItemPalette>::const

  private:
    static std::unordered_map<std::string, ItemPalette> itemPalettes;

    static bool hasPaletteWithName(const std::string &name);
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