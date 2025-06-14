#include "tile.h"

#include <numeric>
#include <ranges>

#include "brushes/border_brush.h"
#include "brushes/brushes.h"
#include "brushes/creature_brush.h"
#include "brushes/doodad_brush.h"
#include "brushes/ground_brush.h"
#include "brushes/mountain_brush.h"
#include "brushes/raw_brush.h"
#include "brushes/wall_brush.h"
#include "items.h"
#include "tile_location.h"

void swap(Tile &first, Tile &second)
{
    using std::swap;

    DEBUG_ASSERT(first._position == second._position, "Can not swap tiles with different positions.");

    swap(first._items, second._items);
    swap(first._ground, second._ground);
    swap(first._creature, second._creature);
    swap(first._selectionCount, second._selectionCount);
    swap(first._flags, second._flags);
}

Tile::Tile(TileLocation &tileLocation)
    : _position(tileLocation.position()), _flags(0), _selectionCount(0) {}

Tile::Tile(Position position)
    : _position(position), _flags(0), _selectionCount(0) {}

Tile::Tile(Tile &&other) noexcept
    : _items(std::move(other._items)),
      _ground(std::move(other._ground)),
      _creature(std::move(other._creature)),
      _position(other._position),
      _flags(other._flags),
      _selectionCount(other._selectionCount)
{
    other._selectionCount = 0;
    other._flags = 0;
}

Tile &Tile::operator=(Tile &&other) noexcept
{
    _items = std::move(other._items);
    _ground = std::move(other._ground);
    _creature = std::move(other._creature);
    _position = std::move(other._position);
    _selectionCount = other._selectionCount;
    _flags = other._flags;

    other._selectionCount = 0;
    other._flags = 0;
    return *this;
}

void Tile::setLocation(TileLocation &location)
{
    _position = location.position();
}

void Tile::setCreature(Creature &&creature)
{
    setCreature(std::make_shared<Creature>(std::move(creature)));
}

void Tile::setCreature(std::shared_ptr<Creature> &&creature)
{
    if (_creature)
    {
        removeCreature();
    }

    if (!creature)
    {
        return;
    }

    if (creature->selected)
    {
        ++_selectionCount;
    }
    _creature = std::move(creature);
}

Creature *Tile::replaceCreature(Creature &&creature)
{
    bool currentSelected = _creature && _creature->selected;

    if (currentSelected && !creature.selected)
    {
        --_selectionCount;
    }
    else if (!currentSelected && creature.selected)
    {
        ++_selectionCount;
    }

    _creature = std::make_shared<Creature>(std::move(creature));
    return &(*_creature);
}

std::shared_ptr<Creature> Tile::dropCreature()
{
    if (!_creature)
    {
        return {};
    }

    if (_creature->selected)
    {
        --_selectionCount;
    }
    std::shared_ptr<Creature> droppedCreature(std::move(_creature));
    _creature.reset();

    return droppedCreature;
}

void Tile::removeItem(size_t index)
{
    deselectItemAtIndex(index);
    _items.erase(_items.begin() + index);
}

void Tile::clearBottomItems()
{
    if (_items.empty())
        return;

    using iterator = std::vector<std::shared_ptr<Item>>::iterator;
    bool hasStart = false;
    iterator start;
    auto it = _items.begin();

    for (; it != _items.end(); ++it)
    {
        ItemType *itemType = (*it)->itemType;
        if (!itemType->isBorder())
        {
            if (!hasStart)
            {
                start = it;
                hasStart = true;
            }

            if (!itemType->isBottom())
            {
                break;
            }
        }
    }

    if (hasStart)
    {
        _items.erase(start, it);
    }
}

void Tile::clearBorders()
{
    auto it = _items.begin();
    while (it != _items.end())
    {
        if (!(*it)->itemType->isBorder())
        {
            _items = {it, _items.end()};
            return;
        }

        ++it;
    }
}

void Tile::clearAll()
{
    _items.clear();
    _ground.reset();
    _creature.reset();
    _selectionCount = 0;
}

void Tile::clearItems()
{
    auto k = std::ranges::count_if(_items, [](const std::shared_ptr<Item> &item) { return item->selected; });
    _items.clear();
    _selectionCount -= k;
}

GroundBrush *Tile::groundBrush() const
{
    if (_ground)
    {
        Brush *brush = _ground->itemType->getBrush(BrushType::Ground);
        if (brush)
        {
            return static_cast<GroundBrush *>(brush);
        }
        else
        {
            return nullptr;
        }
    }
    else
    {
        return nullptr;
    }
}

void Tile::removeItem(Item *item)
{
    if (_ground.get() == item)
    {
        removeGround();
        return;
    }

    auto found = findItem([item](const Item &_item) { return item == &_item; });

    if (found != _items.end())
    {
        auto index = static_cast<size_t>(found - _items.begin());
        removeItem(index);
    }
    else
    {
        ABORT_PROGRAM("[Tile::removeItem] The item was not present in the tile.");
    }
}

void Tile::removeItem(std::function<bool(const Item &)> predicate)
{
    auto found = findItem(predicate);
    if (found != _items.end())
    {
        removeItem(found - _items.begin());
    }
}

std::shared_ptr<Item> Tile::dropItem(size_t index)
{
    std::shared_ptr<Item> item(std::move(_items.at(index)));
    _items.erase(_items.begin() + index);

    if (item->selected)
        _selectionCount -= 1;

    return item;
}

std::shared_ptr<Item> Tile::dropItem(Item *item)
{
    DEBUG_ASSERT(!(_ground && item == _ground.get()), "Can not drop ground using dropItem (as of now. It will maybe make sense in the future to be able to do so).");

    auto found = findItem([item](const Item &_item) { return item == &_item; });

    if (found == _items.end())
    {
        ABORT_PROGRAM("[Tile::dropItem] The item was not present in the tile.");
    }

    auto index = static_cast<size_t>(found - _items.begin());
    return dropItem(index);
}

void Tile::deselectAll()
{
    if (_ground)
        _ground->selected = false;

    if (_creature)
        _creature->selected = false;

    for (auto &item : _items)
    {
        item->selected = false;
    }

    _selectionCount = 0;
}

void Tile::moveItems(Tile &other)
{
    int numSelected = 0;
    for (auto &item : _items)
    {
        if (item->selected)
        {
            ++numSelected;
        }

        other.addItem(std::move(item));
    }
    _items.clear();

    _selectionCount -= numSelected;
}

void Tile::moveItemsWithBroadcast(Tile &other)
{
    int numSelected = 0;
    for (auto &item : _items)
    {
        if (item->selected)
        {
            ++numSelected;
        }

        Item *newItem = other.addItem(std::move(item));
        Items::items.itemAddressChanged(newItem);
    }
    _items.clear();

    _selectionCount -= numSelected;
}

void Tile::moveSelected(Tile &other)
{
    if (_ground && _ground->selected)
    {
        other.clearItems();
        other.removeCreature();
        other.setGround(dropGround());
    }

    if (_creature && _creature->selected)
    {
        other.setCreature(dropCreature());
    }

    std::erase_if(_items, [&](const auto &item) {
        if (item->selected)
        {
            other.addItem(std::move(item));
            --_selectionCount;
            return true;
        }
        return false;
    });
}

Item *Tile::itemAt(size_t index)
{
    return index >= _items.size() ? nullptr : _items.at(index).get();
}

Item *Tile::itemAt(size_t index) const
{
    return index >= _items.size() ? nullptr : _items.at(index).get();
}

void Tile::insertItem(std::shared_ptr<Item> item, size_t index)
{
    _items.emplace(_items.begin() + index, item);
}

void Tile::insertItem(Item &&item, size_t index)
{
    _items.emplace(_items.begin() + index, std::make_shared<Item>(std::move(item)));
}

Item *Tile::addItem(uint32_t serverId)
{
    return addItem(Item(serverId));
}

Item *Tile::addItem(std::shared_ptr<Item> item)
{
    if (item->isGround())
    {
        VME_LOG_D("TODO ::: Tile::addItem: Did nothing!");
        // return replaceGround(std::move(item));
        return _ground.get();
    }

    if (item->selected)
    {
        ++_selectionCount;
    }

    if (_items.size() == 0 || item->isTop() || item->itemType->stackOrder >= _items.back()->itemType->stackOrder)
    {
        auto &newItem = _items.emplace_back(item);
        return newItem.get();
    }
    else
    {

        auto cursor = _items.begin();
        while (cursor != _items.end() && (*cursor)->itemType->stackOrder <= item->itemType->stackOrder)
        {
            ++cursor;
        }

        if (cursor == _items.end())
        {
            auto &newItem = _items.emplace_back(item);
            return newItem.get();
        }
        else
        {
            auto &newItem = *_items.emplace(cursor, item);
            return newItem.get();
        }
    }
}

Item *Tile::addBorder(Item &&item, uint32_t zOrder)
{
    DEBUG_ASSERT(item.isBorder(), "addBorder() called on a non-border.");

    if (item.selected)
        ++_selectionCount;

    if (_items.size() == 0)
    {
        auto &newItem = _items.emplace_back(std::make_unique<Item>(std::move(item)));
        return newItem.get();
    }

    auto cursor = _items.begin();
    while (cursor != _items.end() && ((*cursor)->itemType->isBorder() || (*cursor)->itemType->stackOrder <= TileStackOrder::Border))
    {
        Brush *brush = (*cursor)->itemType->getBrush(BrushType::Border);
        if (brush)
        {
            uint32_t brushZOrder = static_cast<BorderBrush *>(brush)->preferredZOrder();
            if (zOrder < brushZOrder)
            {
                // Found our spot
                break;
            }
        }

        ++cursor;
    }

    auto &newItem = *_items.emplace(cursor, std::make_unique<Item>(std::move(item)));
    return newItem.get();
}

Item *Tile::addItem(Item &&item, int insertionOffset)
{
    DEBUG_ASSERT(insertionOffset >= 0, "insertionOffset must be >= 0.");

    if (item.isGround())
    {
        return replaceGround(std::move(item));
    }

    if (item.selected)
        ++_selectionCount;

    if (_items.empty() || item.isTop() || item.itemType->stackOrder >= _items.back()->itemType->stackOrder)
    {
        if (insertionOffset == 0)
        {
            auto &newItem = _items.emplace_back(std::make_unique<Item>(std::move(item)));
            return newItem.get();
        }

        auto cursor = _items.end() - std::min(insertionOffset, static_cast<int>(_items.size()));

        auto &newItem = *_items.emplace(cursor, std::make_unique<Item>(std::move(item)));
        return newItem.get();
    }
    else
    {
        auto cursor = _items.begin();
        while (cursor != _items.end() && (*cursor)->itemType->stackOrder <= item.itemType->stackOrder)
        {
            ++cursor;
        }

        if (cursor == _items.end())
        {
            auto &newItem = _items.emplace_back(std::make_unique<Item>(std::move(item)));
            return newItem.get();
        }
        else
        {
            auto s = static_cast<int>(std::distance(_items.begin(), cursor));
            cursor -= std::min(insertionOffset, s);

            cursor -= insertionOffset;
            if (cursor < _items.begin())
            {
                cursor = _items.begin();
            }

            auto &newItem = *_items.emplace(cursor, std::make_unique<Item>(std::move(item)));
            return newItem.get();
        }
    }
}

Item *Tile::replaceGround(Item &&ground)
{
    bool currentSelected = _ground && _ground->selected;

    if (currentSelected && !ground.selected)
    {
        --_selectionCount;
    }
    else if (!currentSelected && ground.selected)
    {
        ++_selectionCount;
    }

    _ground = std::make_shared<Item>(std::move(ground));
    return &(*_ground);
}

Item *Tile::replaceItem(size_t index, Item &&item)
{
    bool s1 = _items.at(index)->selected;
    bool s2 = item.selected;
    _items.at(index) = std::make_shared<Item>(std::move(item));

    if (s1 && !s2)
        --_selectionCount;
    else if (!s1 && s2)
        ++_selectionCount;

    return _items.at(index).get();
}

// TODO Might be incorrect?
void Tile::replaceItemByServerId(uint32_t serverId, uint32_t newServerId)
{
    auto found = std::find_if(_items.begin(), _items.end(), [serverId](const std::shared_ptr<Item> &item) { return item->serverId() == serverId; });
    if (found != _items.end())
    {
        if ((*found)->selected)
        {
            --_selectionCount;
        }

        auto newItem = std::make_shared<Item>(newServerId);
        found->swap(newItem);
    }
}

void Tile::setGround(std::shared_ptr<Item> ground)
{
    DEBUG_ASSERT(ground->isGround(), "Tried to add a ground that is not a ground item.");

    if (_ground)
        removeGround();

    if (ground->selected)
        ++_selectionCount;

    _ground = std::move(ground);
}

void Tile::removeGround()
{
    if (!_ground)
    {
        return;
    }

    if (_ground->selected)
    {
        --_selectionCount;
    }
    _ground.reset();
}

void Tile::removeSelectedThings()
{
    if (_ground && _ground->selected)
    {
        removeGround();
    }

    if (_creature && _creature->selected)
    {
        removeCreature();
    }

    auto erased = std::erase_if(_items, [](const std::shared_ptr<Item> &item) { return item->selected; });
    _selectionCount -= erased;
}

void Tile::removeCreature()
{
    if (!_creature)
    {
        return;
    }

    if (_creature->selected)
    {
        --_selectionCount;
    }
    _creature.reset();
}

void Tile::setItemSelected(size_t itemIndex, bool selected)
{
    if (selected)
        selectItemAtIndex(itemIndex);
    else
        deselectItemAtIndex(itemIndex);
}

void Tile::selectItemAtIndex(size_t index)
{
    if (!_items.at(index)->selected)
    {
        _items.at(index)->selected = true;
        ++_selectionCount;
    }
}

void Tile::deselectItemAtIndex(size_t index)
{
    if (_items.at(index)->selected)
    {
        _items.at(index)->selected = false;
        --_selectionCount;
    }
}

void Tile::selectAll()
{
    size_t count = 0;
    if (_ground)
    {
        ++count;
        _ground->selected = true;
    }

    if (_creature)
    {
        ++count;
        _creature->selected = true;
    }

    count += _items.size();
    for (auto &item : _items)
    {
        item->selected = true;
    }

    DEBUG_ASSERT(count < UINT16_MAX, "Count too large.");
    _selectionCount = static_cast<uint16_t>(count);
}

void Tile::setGroundSelected(bool selected)
{
    if (selected)
        selectGround();
    else
        deselectGround();
}

void Tile::setCreatureSelected(bool selected)
{
    if (selected)
    {
        if (_creature && !_creature->selected)
        {
            ++_selectionCount;
            _creature->selected = true;
        }
    }
    else
    {
        if (_creature && _creature->selected)
        {
            --_selectionCount;
            _creature->selected = false;
        }
    }
}

void Tile::selectGround()
{
    if (_ground && !_ground->selected)
    {
        ++_selectionCount;
        _ground->selected = true;
    }
}
void Tile::deselectGround()
{
    if (_ground && _ground->selected)
    {
        --_selectionCount;
        _ground->selected = false;
    }
}

std::shared_ptr<Item> Tile::dropGround()
{
    if (_ground)
    {
        if (_ground->selected)
        {
            --_selectionCount;
        }
        std::shared_ptr<Item> ground = std::move(_ground);
        _ground.reset();

        return ground;
    }
    else
    {
        return {};
    }
}

void Tile::selectTopItem()
{
    if (_items.empty())
    {
        selectGround();
    }
    else
    {
        selectItemAtIndex(_items.size() - 1);
    }
}

void Tile::deselectTopItem()
{
    if (_items.empty())
    {
        deselectGround();
    }
    else
    {
        deselectItemAtIndex(_items.size() - 1);
    }
}

bool Tile::hasTopItem() const
{
    return !isEmpty();
}

Item *Tile::getTopItem() const
{
    if (_items.size() > 0)
    {
        return const_cast<Item *>(_items.back().get());
    }
    if (_ground)
    {
        return _ground.get();
    }

    return nullptr;
}

TileThing Tile::getTopThing() const
{
    if (hasCreature())
    {
        return _creature.get();
    }
    else
    {
        return getTopItem();
    }
}

bool Tile::hasBlockingItem() const noexcept
{
    if (!_ground || _ground->itemType->isBlocking())
    {
        return true;
    }

    for (const auto &item : std::ranges::views::reverse(_items))
    {
        if (item->itemType->isBlocking())
        {
            return true;
        }
    }

    return false;
}

uint8_t Tile::minimapColor() const
{
    for (const auto &item : std::ranges::views::reverse(_items))
    {
        uint8_t color = item->minimapColor();
        if (color != 0)
        {
            return color;
        }
    }

    return hasGround() ? _ground->minimapColor() : 0;
}

bool Tile::topThingSelected() const
{
    if (hasCreature())
    {
        return _creature->selected;
    }

    return topItemSelected();
}

bool Tile::topItemSelected() const
{
    if (!hasTopItem())
        return false;

    const Item *topItem = getTopItem();
    return topItem->selected;
}

size_t Tile::getEntityCount()
{
    size_t result = _items.size();
    if (_ground)
    {
        ++result;
    }

    if (_creature)
    {
        ++result;
    }

    return result;
}

std::vector<Brush *> Tile::getTopBrushes()
{
    std::vector<Brush *> brushes;
    // Raw brush
    auto topItem = getTopItem();
    if (topItem)
    {
        uint32_t id = topItem->serverId();
        Brush *brush = static_cast<Brush *>(Brushes::getOrCreateRawBrush(id));
        brushes.emplace_back(brush);
    }

    // Wall brush
    WallBrush *wallBrush = Brushes::getWallBrush(*this);
    if (wallBrush)
    {
        brushes.emplace_back(static_cast<Brush *>(wallBrush));
    }

    // Border brush
    BorderBrush *borderBrush = Brushes::getBorderBrush(*this);
    if (borderBrush)
    {
        brushes.emplace_back(static_cast<Brush *>(borderBrush));
    }

    // Doodad brush
    DoodadBrush *doodadBrush = Brushes::getDoodadBrush(*this);
    if (doodadBrush)
    {
        brushes.emplace_back(static_cast<Brush *>(doodadBrush));
    }

    // Ground brush
    GroundBrush *groundBrush = Brushes::getGroundBrush(*this);
    if (groundBrush)
    {
        brushes.emplace_back(static_cast<Brush *>(groundBrush));
    }

    // Mountain brush
    MountainBrush *mountainBrush = Brushes::getMountainBrush(*this);
    if (mountainBrush)
    {
        brushes.emplace_back(static_cast<Brush *>(mountainBrush));
    }

    // Creature brush
    CreatureBrush *creatureBrush = Brushes::getCreatureBrush(*this);
    if (creatureBrush)
    {
        brushes.emplace_back(static_cast<Brush *>(creatureBrush));
    }

    return brushes;
}

int Tile::getTopElevation() const
{
    return std::accumulate(
        _items.begin(),
        _items.end(),
        0,
        [](int elevation, const std::shared_ptr<Item> &next) { return elevation + (*next).itemType->getElevation(); });
}

Tile Tile::copyForHistory() const
{
    Tile tile(_position);

    tile._items = _items;

    if (_ground)
    {
        tile._ground = _ground;
    }

    if (_creature)
    {
        tile._creature = _creature;
    }

    tile._flags = this->_flags;
    tile._selectionCount = this->_selectionCount;

    return tile;
}

Tile Tile::deepCopy(Position newPosition) const
{
    Tile tile(newPosition);
    deepCopyInto(tile, false);
    return tile;
}

void Tile::moveAllExceptPosition(Tile &source, Tile &target)
{
    target._items = std::move(source._items);
    target._ground = std::move(source._ground);
    target._creature = std::move(source._creature);
    target._selectionCount = source._selectionCount;
    target._flags = source._flags;
}

Tile Tile::deepCopy(bool onlySelected) const
{
    Tile tile(_position);
    deepCopyInto(tile, onlySelected);
    return tile;
}

void Tile::merge(const Tile &tile)
{
    applyToBase(*this, tile, false);
}

void Tile::applyToBase(Tile &base, const Tile &other, bool onlySelected)
{
    for (const auto &item : other._items)
    {
        if (!onlySelected || item->selected)
        {
            base.addItem(item->deepCopy());
        }
    }

    if (other._ground && (!onlySelected || other._ground->selected))
    {
        base.replaceGround(other._ground->deepCopy());
        base._flags = other._flags;
    }

    if (other._creature && (!onlySelected || other._creature->selected))
    {
        base.replaceCreature(other._creature->deepCopy());
    }
}

void Tile::deepCopyInto(Tile &tile, bool onlySelected) const
{
    applyToBase(tile, *this, onlySelected);
}

bool Tile::isEmpty() const
{
    return !_ground && _items.empty() && !_creature;
}

bool Tile::hasItems() const
{
    return _ground || !_items.empty();
}

bool Tile::allSelected() const
{
    size_t thingCount = _items.size();
    if (_ground)
        ++thingCount;

    if (_creature)
        ++thingCount;

    return _selectionCount == thingCount;
}

bool Tile::hasSelection() const
{
    return _selectionCount != 0;
}

Item *Tile::firstSelectedItem()
{
    const Item *item = static_cast<const Tile *>(this)->firstSelectedItem();
    return const_cast<Item *>(item);
}

const Item *Tile::firstSelectedItem() const
{
    if (_selectionCount == 0)
        return nullptr;

    if (_ground && _ground->selected)
        return ground();

    for (auto &item : _items)
    {
        if (item->selected)
            return item.get();
    }

    return nullptr;
}

TileThing Tile::firstSelectedThing()
{
    if (_creature && _creature->selected)
    {
        return _creature.get();
    }
    else
    {
        Item *item = firstSelectedItem();
        return item;
    }
}

TileThing Tile::getThing(int offsetFromTop) const
{
    // Creature?
    if (offsetFromTop == 0 && _creature)
    {
        return _creature.get();
    }

    int index = _items.size() - 1 - offsetFromTop;

    if (index >= 0)
    {
        return itemAt(index);
    }

    if (_ground)
    {
        return _ground.get();
    }

    // If there's no ground, return the bottom item if we have one
    if (!_items.empty())
    {
        return _items.front().get();
    }

    return std::monostate{};
}

void Tile::setFlags(uint32_t flags)
{
    _flags = flags;
}

bool Tile::containsItem(std::function<bool(const Item &)> predicate) const
{
    return findItem(predicate) != _items.end();
}

const std::vector<std::shared_ptr<Item>>::const_iterator Tile::findItem(std::function<bool(const Item &)> predicate) const
{
    return std::find_if(_items.begin(), _items.end(), [predicate](const std::shared_ptr<Item> &item) { return predicate(*item); });
}

const Item *Tile::getItem(std::function<bool(const Item &)> predicate) const
{
    auto found = findItem(predicate);
    return (found != _items.end()) ? (*found).get() : nullptr;
}

std::optional<size_t> Tile::indexOf(Item *item) const
{
    auto found = findItem([item](const Item &_item) { return item == &_item; });
    if (found != _items.end())
    {
        return static_cast<size_t>(found - _items.begin());
    }
    else
    {
        return std::nullopt;
    }
}

void Tile::movedInMap()
{
    if (_ground)
    {
        Items::items.itemAddressChanged(&(*_ground));
    }

    for (auto &item : _items)
    {
        Items::items.itemAddressChanged(item.get());
    }
}

void Tile::swapCreature(std::shared_ptr<Creature> &creature)
{
    _creature.swap(creature);
}

bool Tile::hasWall(const WallBrush *brush) const
{
    for (auto it = _items.rbegin(); it != _items.rend(); ++it)
    {
        if (brush->includes((*it)->serverId()))
        {
            return true;
        }
        // Borders should always be below walls
        else if ((*it)->itemType->isBorder())
        {
            return false;
        }
    }

    return false;
}

bool Tile::containsBorder(const BorderBrush *brush) const
{
    Brush *centerBrush = brush->centerBrush();
    if (centerBrush && _ground && centerBrush->erasesItem(_ground->serverId()))
    {
        return true;
    }

    for (const auto &item : _items)
    {
        if (brush->includes(item->serverId()))
        {
            return true;
        }
        else if (!item->itemType->isBorder())
        {
            return false;
        }
    }

    return false;
}

uint32_t Tile::getBorderServerId(const BorderBrush *brush) const
{
    Brush *centerBrush = brush->centerBrush();
    if (centerBrush && _ground && centerBrush->erasesItem(_ground->serverId()))
    {
        return _ground->serverId();
    }

    for (const auto &item : _items)
    {
        if (brush->includes(item->serverId()))
        {
            return item->serverId();
        }
        else if (!item->itemType->isBorder())
        {
            return 0;
        }
    }

    return 0;
}

TileCover Tile::getTileCover(const BorderBrush *brush) const
{
    Brush *centerBrush = brush->centerBrush();
    if (centerBrush && _ground && centerBrush->erasesItem(_ground->serverId()))
    {
        return TILE_COVER_FULL;
    }

    TileCover result = TILE_COVER_NONE;

    for (const auto &item : _items)
    {
        if (brush->includes(item->serverId()))
        {
            result |= brush->getTileCover(item->serverId());
        }
        else if (!item->itemType->isBorder())
        {
            return result;
        }
    }

    return result;
}

TileCover Tile::getTileCover(const MountainBrush *brush) const
{
    Brush *centerBrush = brush->ground();
    if (centerBrush && _ground && centerBrush->erasesItem(_ground->serverId()))
    {
        return TILE_COVER_FULL;
    }

    TileCover result = TILE_COVER_NONE;

    for (const auto &item : _items)
    {
        if (brush->erasesItem(item->serverId()))
        {
            result |= brush->getTileCover(item->serverId());
        }
    }

    return result;
}

TileBorderBlock Tile::getFullBorderTileCover(TileCover excludeMask) const
{
    TileBorderBlock block;
    if (_ground)
    {
        ItemType *itemType = _ground->itemType;
        // Some borders, like shallow water, are ground items
        // if (itemType->hasFlag(ItemTypeFlag::InBorderBrush))
        // {
        //     auto brush = itemType->getBrush(BrushType::Border);

        //     if (brush)
        //     {
        //         auto borderBrush = static_cast<BorderBrush *>(brush);
        //         auto cover = borderBrush->getTileCover(itemType->id);
        //         cover &= ~(excludeMask);

        //         if (cover != TILE_COVER_NONE)
        //         {
        //             block.add(cover, borderBrush);
        //         }
        //     }
        // }
        if (itemType->hasFlag(ItemTypeFlag::InGroundBrush | ItemTypeFlag::InMountainBrush))
        {
            Brush *brush = itemType->brush();

            // We might have to get the ground from a mountain brush
            if (brush->type() == BrushType::Mountain)
            {
                brush = static_cast<MountainBrush *>(brush)->ground();
            }

            if (brush->type() == BrushType::Ground)
            {
                auto *groundBrush = static_cast<GroundBrush *>(brush);
                block.ground = groundBrush;
            }
        }
    }

    for (const auto &item : _items)
    {
        ItemType *itemType = item->itemType;

        if (!itemType->isBorder())
            return block;

        auto brush = itemType->getBrush(BrushType::Border);

        if (brush)
        {
            auto borderBrush = static_cast<BorderBrush *>(brush);
            auto cover = borderBrush->getTileCover(item->serverId());
            cover &= ~(excludeMask);

            if (cover != TILE_COVER_NONE)
            {
                block.add(cover, borderBrush);
            }
        }
    }

    return block;
}

BorderCover::BorderCover(TileCover cover, BorderBrush *brush)
    : cover(cover), brush(brush)
{
    // Necessary to make sure center is instantiated
    // Brush *center = brush->centerBrush();

    DEBUG_ASSERT(brush != nullptr, "nullptr brush");
}