#include "tile.h"

#include <numeric>

#include "ecs/ecs.h"
#include "ecs/item_animation.h"
#include "tile_location.h"
#include "items.h"

Tile::Tile(TileLocation &tileLocation)
    : _position(tileLocation.position()), _flags(0), _selectionCount(0) {}

Tile::Tile(Position position)
    : _position(position), _flags(0), _selectionCount(0) {}

Tile::Tile(Tile &&other) noexcept
    : _items(std::move(other._items)),
      _ground(std::move(other._ground)),
      _position(other._position),
      _creature(std::move(other._creature)),
      _flags(other._flags),
      _selectionCount(other._selectionCount) {}

Tile &Tile::operator=(Tile &&other) noexcept
{
  _items = std::move(other._items);
  _ground = std::move(other._ground);
  _position = std::move(other._position);
  _creature = std::move(other._creature);
  _selectionCount = other._selectionCount;
  _flags = other._flags;

  return *this;
}

void Tile::setLocation(TileLocation &location)
{
  _position = location.position();
}

void Tile::setCreature(Creature &&creature)
{
  _creature = std::make_unique<Creature>(std::move(creature));
}

void Tile::setCreature(std::unique_ptr<Creature> &&creature)
{
  _creature = std::move(creature);
}

std::unique_ptr<Creature> Tile::dropCreature()
{
  std::unique_ptr<Creature> droppedCreature(std::move(_creature));
  _creature.reset();

  return droppedCreature;
}

void Tile::removeItem(size_t index)
{
  deselectItemAtIndex(index);
  _items.erase(_items.begin() + index);
}

void Tile::removeItem(Item *item)
{
  if (_ground.get() == item)
  {
    removeGround();
    return;
  }

  auto found = findItem([item](const Item &_item) {
    return item == &_item;
  });

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

Item Tile::dropItem(size_t index)
{
  Item item = std::move(_items.at(index));
  _items.erase(_items.begin() + index);

  if (item.selected)
    _selectionCount -= 1;

  return item;
}

Item Tile::dropItem(Item *item)
{
  DEBUG_ASSERT(!(_ground && item == _ground.get()), "Can not drop ground using dropItem (as of now. It will maybe make sense in the future to be able to do so).");

  auto found = findItem([item](const Item &_item) {
    return item == &_item;
  });

  if (found != _items.end())
  {
    auto index = static_cast<size_t>(found - _items.begin());
    return dropItem(index);
  }

  ABORT_PROGRAM("[Tile::dropItem] The item was not present in the tile.");
}

void Tile::deselectAll()
{
  if (_ground)
    _ground->selected = false;

  for (Item &item : _items)
  {
    item.selected = false;
  }

  _selectionCount = 0;
}

void Tile::moveItems(Tile &other)
{
  for (auto &item : _items)
  {
    other.addItem(std::move(item));
  }
  _items.clear();

  _selectionCount = (_ground && _ground->selected) ? 1 : 0;
}

void Tile::moveSelected(Tile &other)
{
  if (_ground && _ground->selected)
  {
    other._items.clear();
    other._ground = dropGround();
  }

  auto it = _items.begin();
  while (it != _items.end())
  {
    Item &item = *it;
    if (item.selected)
    {
      other.addItem(std::move(item));
      it = _items.erase(it);

      --_selectionCount;
    }
    else
    {
      ++it;
    }
  }
}

Item *Tile::itemAt(size_t index)
{
  return index >= _items.size() ? nullptr : &_items.at(index);
}

void Tile::insertItem(Item &&item, size_t index)
{
  _items.emplace(_items.begin() + index, std::move(item));
}

void Tile::addItem(Item &&item)
{
  if (item.isGround())
  {
    replaceGround(std::move(item));
    return;
  }

  ItemType &itemType = *item.itemType;

  // Place item on top of tile if it does not want to be on bottom.
  if (!itemType.alwaysBottomOfTile)
  {
    if (item.selected)
      ++_selectionCount;

    _items.emplace(_items.end(), std::move(item));
    return;
  }

  bool replace = false;
  bool border = itemType.isGroundBorder();

  auto cursor = _items.begin();
  for (; cursor != _items.end(); ++cursor)
  {
    ItemType &currentType = *cursor->itemType;
    if (!currentType.alwaysBottomOfTile)
      break;

    if (itemType.isGroundBorder() && !currentType.isGroundBorder())
      break;

    if (!currentType.isGroundBorder())
    {
      // Replace the current item at cursor with the new item
      size_t index = static_cast<size_t>(cursor - _items.begin());
      replaceItem(index, std::move(item));
      return;
    }
  }

  if (item.selected)
    ++_selectionCount;

  _items.emplace(cursor, std::move(item));
}

void Tile::replaceGround(Item &&ground)
{
  bool currentSelected = _ground && _ground->selected;

  if (currentSelected && !ground.selected)
    --_selectionCount;
  else if (!currentSelected && ground.selected)
    ++_selectionCount;

  _ground = std::make_unique<Item>(std::move(ground));
}

void Tile::replaceItem(size_t index, Item &&item)
{
  bool s1 = _items.at(index).selected;
  bool s2 = item.selected;
  _items.at(index) = std::move(item);

  if (s1 && !s2)
    --_selectionCount;
  else if (!s1 && s2)
    ++_selectionCount;
}

void Tile::setGround(std::unique_ptr<Item> ground)
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
  if (_ground->selected)
  {
    --_selectionCount;
  }
  _ground.reset();
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
  if (!_items.at(index).selected)
  {
    _items.at(index).selected = true;
    ++_selectionCount;
  }
}

void Tile::deselectItemAtIndex(size_t index)
{
  if (_items.at(index).selected)
  {
    _items.at(index).selected = false;
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

  count += _items.size();
  for (Item &item : _items)
  {
    item.selected = true;
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

std::unique_ptr<Item> Tile::dropGround()
{
  if (_ground)
  {
    std::unique_ptr<Item> ground = std::move(_ground);
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
    return const_cast<Item *>(&_items.back());
  }
  if (_ground)
  {
    return _ground.get();
  }

  return nullptr;
}

bool Tile::topItemSelected() const
{
  if (!hasTopItem())
    return false;

  const Item *topItem = getTopItem();
  return allSelected() || topItem->selected;
}

size_t Tile::getEntityCount()
{
  size_t result = _items.size();
  if (_ground)
    ++result;

  return result;
}

int Tile::getTopElevation() const
{
  return std::accumulate(
      _items.begin(),
      _items.end(),
      0,
      [](int elevation, const Item &next) { return elevation + next.itemType->getElevation(); });
}

Tile Tile::deepCopy() const
{
  Tile tile(_position);
  for (const auto &item : _items)
  {
    tile.addItem(item.deepCopy());
  }
  tile._flags = this->_flags;
  if (_ground)
  {
    tile._ground = std::make_unique<Item>(_ground->deepCopy());
  }

  if (_creature)
  {
    tile._creature = std::make_unique<Creature>(_creature->deepCopy());
  }

  tile._selectionCount = this->_selectionCount;

  return tile;
}

bool Tile::isEmpty() const
{
  return !_ground && _items.empty() && !_creature;
}

bool Tile::allSelected() const
{
  size_t size = _items.size();
  if (_ground)
    ++size;

  return _selectionCount == size;
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
  if (_ground && _ground->selected)
    return ground();

  for (auto &item : _items)
  {
    if (item.selected)
      return &item;
  }

  return nullptr;
}

void Tile::setFlags(uint32_t flags)
{
  _flags = flags;
}

const std::vector<Item>::const_iterator Tile::findItem(std::function<bool(const Item &)> predicate) const
{
  return std::find_if(_items.begin(), _items.end(), predicate);
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

void Tile::initEntities()
{
  if (_ground)
    _ground->registerEntity();

  for (auto &item : _items)
    item.registerEntity();
}

void Tile::destroyEntities()
{
  if (_ground)
    _ground->destroyEntity();

  for (auto &item : _items)
    item.destroyEntity();
}
