#include "item_palette.h"

#include <ranges>
#include <utility>

#include "logger.h"

//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>ItemPalettes>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>
std::unordered_map<std::string, std::shared_ptr<ItemPalette>> ItemPalettes::_itemPalettes;

std::shared_ptr<ItemPalette> ItemPalettes::createPalette(const std::string &id, const std::string &name)
{
    if (contains(id))
    {
        VME_LOG_ERROR("A palette with id " << id << " is already registered.");
        return _itemPalettes.at(id);
    }

    // Must use 'new' keyword because constructor is private
    auto result = _itemPalettes.try_emplace(id, std::shared_ptr<ItemPalette>(new ItemPalette(id, name)));
    return result.first->second;
}

std::vector<std::shared_ptr<ItemPalette>> ItemPalettes::getItemPalettes()
{
    auto valuesView = std::views::values(_itemPalettes);

    std::vector<std::shared_ptr<ItemPalette>> values{valuesView.begin(), valuesView.end()};
    return values;
}

std::vector<NamedId> ItemPalettes::getItemPaletteList()
{
    auto result = std::views::values(_itemPalettes) |
                  std::ranges::views::transform([](const std::shared_ptr<ItemPalette> &x) {
                      return NamedId{.id = x->id(), .name = x->name()};
                  });

    return std::vector<NamedId>{result.begin(), result.end()};
}

ItemPalette *ItemPalettes::getById(const std::string &id)
{
    auto found = _itemPalettes.find(id);
    return found != _itemPalettes.end() ? found->second.get() : nullptr;
}

ItemPalette *ItemPalettes::getOrCreateById(const std::string &id)
{
    auto found = _itemPalettes.find(id);
    if (found != _itemPalettes.end())
    {
        return found->second.get();
    }
    else
    {
        return createPalette(id, id).get();
    }
}

bool ItemPalettes::contains(const std::string &id)
{
    return _itemPalettes.find(id) != _itemPalettes.end();
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

Tileset &ItemPalette::addTileset(std::unique_ptr<Tileset> tileset)
{
    tileset->setPalette(this);

    tilesetIndexMap.emplace(tileset->id(), _tilesets.size());
    auto &result = _tilesets.emplace_back(std::move(tileset));
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
