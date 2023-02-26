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
    static std::shared_ptr<ItemPalette> createPalette(const std::string &id, const std::string &name);
    static ItemPalette *getById(const std::string &id);
    static ItemPalette *getOrCreateById(const std::string &id);

    static std::vector<std::shared_ptr<ItemPalette>> getItemPaletteList();

  private:
    static std::unordered_map<std::string, std::shared_ptr<ItemPalette>> _itemPalettes;

    static bool contains(const std::string &id);
};
