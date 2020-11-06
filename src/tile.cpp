#include "tile.h"

#include <numeric>

#include "ecs/ecs.h"
#include "ecs/item_animation.h"
#include "tile_location.h"

Tile::Tile(TileLocation &tileLocation)
    : _position(tileLocation.position()), _flags(0), _selectionCount(0) {}

Tile::Tile(Position position)
    : _position(position), _flags(0), _selectionCount(0) {}

Tile::Tile(Tile &&other) noexcept
    : _items(std::move(other._items)),
      _ground(std::move(other._ground)),
      _position(other._position),
      _flags(other._flags),
      _selectionCount(other._selectionCount) {}

Tile &Tile::operator=(Tile &&other) noexcept
{
  _items = std::move(other._items);
  _ground = std::move(other._ground);
  _position = std::move(other._position);
  _selectionCount = other._selectionCount;
  _flags = other._flags;

  return *this;
}

Tile::~Tile()
{
}

void Tile::setLocation(TileLocation &location)
{
  _position = location.position();
}

void Tile::removeItem(size_t index)
{
  deselectItemAtIndex(index);
  _items.erase(_items.begin() + index);
}

Item Tile::dropItem(size_t index)
{
  Item item = std::move(_items.at(index));
  _items.erase(_items.begin() + index);

  if (item.selected)
    _selectionCount -= 1;

  return item;
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

void Tile::addItem(Item &&item)
{
  if (item.isGround())
  {
    replaceGround(std::move(item));
    return;
  }

  std::vector<Item>::iterator it;

  bool replace = false;
  ItemType &itemType = *item.itemType;
  if (itemType.alwaysOnTop)
  {
    for (it = _items.begin(); it != _items.end(); ++it)
    {
      ItemType &currentType = *it->itemType;

      if (currentType.alwaysOnTop)
      {
        if (itemType.isGroundBorder())
        {
          if (!currentType.isGroundBorder())
          {
            break;
          }
        }
        else // New item is not a border
        {
          if (currentType.alwaysOnTop)
          {
            if (!currentType.isGroundBorder())
            {
              // Replace the current item at cursor with the new item
              replace = true;
              break;
            }
          }
        }
        // if (item.getTopOrder() < cursor->getTopOrder())
        // {
        //   break;
        // }
      }
      else
      {
        break;
      }
    }
  }
  else
  {
    it = _items.end();
  }

  if (replace)
  {
    auto offset = it - _items.begin();
    DEBUG_ASSERT(offset < UINT16_MAX, "Offset too large.");
    replaceItem(static_cast<uint16_t>(offset), std::move(item));
  }
  else
  {
    if (item.selected)
      ++_selectionCount;

    _items.emplace(it, std::move(item));
  }
}

void Tile::replaceGround(Item &&ground)
{
  bool s1 = _ground && _ground->selected;
  bool s2 = ground.selected;
  _ground = std::make_unique<Item>(std::move(ground));

  if (s1 && !s2)
    --_selectionCount;
  else if (!s1 && s2)
    ++_selectionCount;
}

void Tile::replaceItem(uint16_t index, Item &&item)
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

Item *Tile::ground() const
{
  return _ground.get();
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

  tile._selectionCount = this->_selectionCount;

  return tile;
}

bool Tile::isEmpty() const
{
  return !_ground && _items.empty();
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
