#include "tile.h"

#include <numeric>

#include "tile_location.h"
#include "ecs/ecs.h"
#include "ecs/item_animation.h"

Tile::Tile(TileLocation &tileLocation)
    : _position(tileLocation.position()), selectionCount(0), flags(0)
{
}

Tile::Tile(Position position)
    : _position(position), selectionCount(0), flags(0) {}

Tile::Tile(Tile &&other) noexcept
    : _position(other._position),
      _ground(std::move(other._ground)),
      items(std::move(other.items)),
      selectionCount(other.selectionCount),
      flags(other.flags)
{
}

Tile &Tile::operator=(Tile &&other) noexcept
{
  items = std::move(other.items);
  _ground = std::move(other._ground);
  _position = std::move(other._position);
  selectionCount = other.selectionCount;
  flags = other.flags;

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
  items.erase(items.begin() + index);
}

void Tile::deselectAll()
{
  if (_ground)
    _ground->selected = false;

  for (Item &item : items)
  {
    item.selected = false;
  }

  selectionCount = 0;
}

void Tile::moveItems(Tile &other)
{
  // TODO
  ABORT_PROGRAM("Not implemented.");
}

void Tile::moveSelected(Tile &other)
{
  if (_ground && _ground->selected)
  {
    other.items.clear();
    other._ground = dropGround();
  }

  auto it = items.begin();
  while (it != items.end())
  {
    Item &item = *it;
    if (item.selected)
    {
      other.addItem(std::move(item));
      it = items.erase(it);

      --selectionCount;
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
    bool oldSelected = _ground && _ground->selected;
    bool newSelected = item.selected;
    _ground = std::make_unique<Item>(std::move(item));
    if (oldSelected && !newSelected)
    {
      --selectionCount;
    }
    else if (newSelected && !oldSelected)
    {
      ++selectionCount;
    }
    return;
  }

  std::vector<Item>::iterator currentItem;

  ItemType &itemType = *item.itemType;
  if (itemType.alwaysOnTop)
  {
    currentItem = items.begin();

    while (currentItem != items.end())
    {
      ItemType &currentType = *currentItem->itemType;

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
              *(currentItem) = std::move(item);
              return;
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
      ++currentItem;
    }
  }
  else
  {
    currentItem = items.end();
  }

  if (item.selected)
  {
    ++selectionCount;
  }

  items.insert(currentItem, std::move(item));
}

void Tile::setGround(std::unique_ptr<Item> ground)
{
  if (!ground)
  {
    _ground = std::move(ground);
  }
  else
  {
    DEBUG_ASSERT(ground->itemType->isGroundTile(), "Tried to add a ground that is not a ground item.");
  }
}

void Tile::removeGround()
{
  if (_ground->selected)
  {
    --selectionCount;
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
  if (!items.at(index).selected)
  {
    items.at(index).selected = true;
    ++selectionCount;
  }
}

void Tile::deselectItemAtIndex(size_t index)
{
  if (items.at(index).selected)
  {
    items.at(index).selected = false;
    --selectionCount;
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

  count += items.size();
  for (Item &item : items)
  {
    item.selected = true;
  }

  selectionCount = count;
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
    ++selectionCount;
    _ground->selected = true;
  }
}
void Tile::deselectGround()
{
  if (_ground && _ground->selected)
  {
    --selectionCount;
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
  if (items.empty())
  {
    selectGround();
  }
  else
  {
    selectItemAtIndex(items.size() - 1);
  }
}

void Tile::deselectTopItem()
{
  if (items.empty())
  {
    deselectGround();
  }
  else
  {
    deselectItemAtIndex(items.size() - 1);
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
  if (items.size() > 0)
  {
    return const_cast<Item *>(&items.back());
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
  size_t result = items.size();
  if (_ground)
    ++result;

  return result;
}

int Tile::getTopElevation() const
{
  return std::accumulate(
      items.begin(),
      items.end(),
      0,
      [](int elevation, const Item &next) { return elevation + next.itemType->getElevation(); });
}

Tile Tile::deepCopy() const
{
  Tile tile(_position);
  for (const auto &item : this->items)
  {
    tile.addItem(item.deepCopy());
  }
  tile.flags = this->flags;
  if (_ground)
  {
    tile._ground = std::make_unique<Item>(_ground->deepCopy());
  }

  tile.selectionCount = this->selectionCount;

  return tile;
}

bool Tile::isEmpty() const
{
  return !_ground && items.empty();
}

bool Tile::allSelected() const
{
  size_t size = items.size();
  if (_ground)
    ++size;

  return selectionCount == size;
}

bool Tile::hasSelection() const
{
  return selectionCount != 0;
}

void Tile::initEntities()
{
  if (_ground)
    _ground->registerEntity();

  for (auto &item : items)
    item.registerEntity();
}

void Tile::destroyEntities()
{
  if (_ground)
    _ground->destroyEntity();

  for (auto &item : items)
    item.destroyEntity();
}
