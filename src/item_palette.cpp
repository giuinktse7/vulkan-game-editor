#include "item_palette.h"

#include <utility>

#include "logger.h"

//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>ItemPalettes>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
std::unordered_map<std::string, ItemPalette> ItemPalettes::itemPalettes;

ItemPalette &ItemPalettes::createPalette(const std::string &name)
{
    if (hasPaletteWithName(name))
    {
        VME_LOG_ERROR("A palette with name " << name << " is already registered.");
        return itemPalettes.at(name);
    }

    auto result = itemPalettes.try_emplace(name, ItemPalette(name));
    return result.first->second;
}

ItemPalette *ItemPalettes::getByName(const std::string &name)
{
    auto found = itemPalettes.find(name);
    return found != itemPalettes.end() ? &(found->second) : nullptr;
}

bool ItemPalettes::hasPaletteWithName(const std::string &name)
{
    return itemPalettes.find(name) != itemPalettes.end();
}

//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>ItemPalette>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>

ItemPalette::ItemPalette(std::string name)
    : _name(name) {}

const std::string &ItemPalette::name() const noexcept
{
    return _name;
}

Tileset *ItemPalette::tileset(const std::string &name)
{
    auto found = tilesetIndexMap.find(name);
    if (found == tilesetIndexMap.end())
        return nullptr;

    size_t index = found->second;
    return _tilesets.at(index).get();
}

Tileset *ItemPalette::createTileset(const std::string &name)
{
    tilesetIndexMap.emplace(name, _tilesets.size());

    std::unique_ptr<Tileset> &tileset = _tilesets.emplace_back(std::make_unique<Tileset>(Tileset(name)));
    tileset->setPalette(this);

    return tileset.get();
}

Tileset *ItemPalette::tileset(size_t index)
{
    return index < _tilesets.size() ? _tilesets.at(index).get() : nullptr;
}

bool ItemPalette::empty() const noexcept
{
    return _tilesets.empty();
}

const std::vector<std::unique_ptr<Tileset>> &ItemPalette::tilesets()
{
    return _tilesets;
}
