#include "item_palette.h"

#include <utility>

#include "logger.h"

//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>ItemPalettes>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
std::unordered_map<std::string, ItemPalette> ItemPalettes::_itemPalettes;

ItemPalette &ItemPalettes::createPalette(const std::string &id, const std::string &name)
{
    if (contains(id))
    {
        VME_LOG_ERROR("A palette with id " << id << " is already registered.");
        return _itemPalettes.at(id);
    }

    auto result = _itemPalettes.try_emplace(id, ItemPalette(id, name));
    return result.first->second;
}

ItemPalette *ItemPalettes::getById(const std::string &id)
{
    auto found = _itemPalettes.find(id);
    return found != _itemPalettes.end() ? &(found->second) : nullptr;
}

bool ItemPalettes::contains(const std::string &id)
{
    return _itemPalettes.find(id) != _itemPalettes.end();
}

std::unordered_map<std::string, ItemPalette> &ItemPalettes::itemPalettes()
{
    return _itemPalettes;
}

//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>ItemPalette>>>>>
//>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>

ItemPalette::ItemPalette(const std::string &id, const std::string &name)
    : _id(id), _name(name) {}

const std::string &ItemPalette::name() const noexcept
{
    return _name;
}

const std::string &ItemPalette::id() const noexcept
{
    return _id;
}

Tileset *ItemPalette::getTileset(const std::string &id)
{
    auto found = tilesetIndexMap.find(id);
    if (found == tilesetIndexMap.end())
        return nullptr;

    size_t index = found->second;
    return _tilesets.at(index).get();
}

Tileset &ItemPalette::addTileset(Tileset &&tileset)
{
    tileset.setPalette(this);

    tilesetIndexMap.emplace(tileset.id(), _tilesets.size());
    auto &result = _tilesets.emplace_back(std::make_unique<Tileset>(std::move(tileset)));
    return *result;
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
