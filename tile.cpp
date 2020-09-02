#include "tile.h"

#include <numeric>

#include "ecs/ecs.h"
#include "tile_location.h"

Tile::Tile(TileLocation &tileLocation)
    : position(tileLocation.position), selectionCount(0)
{
}

Tile::Tile(Position position)
    : position(position), selectionCount(0) {}

// TODO BUG? It is possible that entityId from OptionalEntity does not get moved correctly.
// TODO It should be deleted from "other" and put into the newly constructed Tile.
Tile::Tile(Tile &&other) noexcept
    : items(std::move(other.items)),
      ground(std::move(other.ground)),
      position(other.position),
      selectionCount(other.selectionCount)
{
}

Tile &Tile::operator=(Tile &&other) noexcept
{
  items = std::move(other.items);
  ground = std::move(other.ground);
  position = std::move(other.position);
  selectionCount = other.selectionCount;

  return *this;
}

Tile::~Tile()
{
}

void Tile::setLocation(TileLocation &location)
{
  this->position = location.position;
}

void Tile::removeItem(size_t index)
{
  if (index == items.size() - 1)
  {
    items.pop_back();
  }
  else
  {
    items.erase(items.begin() + index);
  }
}

void Tile::deselectAll()
{
  if (ground)
    ground->selected = false;

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
  if (ground && ground->selected)
  {
    other.items.clear();
    other.ground = dropGround();
  }

  auto it = items.begin();
  while (it != items.end()) {
      Item& item = *it;
      if (item.selected)
      {
          other.addItem(std::move(item));
          it = items.erase(it);

          --selectionCount;
      }
      else {
          ++it;
      }
  }
}

void Tile::addItem(Item &&item)
{
  if (item.isGround())
  {
    bool oldSelected = ground && ground->selected;
    bool newSelected = item.selected;
    this->ground = std::make_unique<Item>(std::move(item));
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

  std::vector<Item>::iterator cursor;

  if (item.itemType->alwaysOnTop)
  {
    cursor = items.begin();
    while (cursor != items.end())
    {
      if (cursor->itemType->alwaysOnTop)
      {
        if (item.itemType->isGroundBorder())
        {
          if (!cursor->itemType->isGroundBorder())
          {
            break;
          }
        }
        else // New item is not a border
        {
          if (cursor->itemType->alwaysOnTop)
          {
            if (!cursor->itemType->isGroundBorder())
            {
              // Replace the current item at cursor with the new item
              *(cursor) = std::move(item);
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
      ++cursor;
    }
  }
  else
  {
    cursor = items.end();
  }

  if (item.selected)
  {
    ++selectionCount;
  }

  items.insert(cursor, std::move(item));
}

void Tile::setGround(std::unique_ptr<Item> ground)
{
  if (!ground)
  {
    this->ground = std::move(ground);
  }
  else
  {
    DEBUG_ASSERT(ground->itemType->isGroundTile(), "Tried to add a ground that is not a ground item.");
  }
}

void Tile::removeGround()
{
  if (ground->selected)
  {
    --selectionCount;
  }
  ground.reset();
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
  if (ground)
  {
    ++count;
    ground->selected = true;
  }

  count += items.size();
  for (Item &item : items)
  {
    item.selected = true;
  }

  selectionCount = count;
}

void Tile::selectGround()
{
  if (ground && !ground->selected)
  {
    ++selectionCount;
    ground->selected = true;
  }
}
void Tile::deselectGround()
{
  if (ground && ground->selected)
  {
    --selectionCount;
    ground->selected = false;
  }
}

std::unique_ptr<Item> Tile::dropGround()
{
  if (ground)
  {
    std::unique_ptr<Item> ground = std::move(this->ground);
    this->ground.reset();

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

Item *Tile::getGround() const
{
  return ground.get();
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
  if (ground)
  {
    return ground.get();
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
  if (ground)
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
  Tile tile(this->position);
  for (const auto &item : this->items)
  {
    tile.addItem(item.deepCopy());
  }
  tile.flags = this->flags;
  if (this->getGround())
  {
    tile.ground = std::make_unique<Item>(this->getGround()->deepCopy());
  }

  tile.selectionCount = this->selectionCount;

  return tile;
}

const Position Tile::getPosition() const
{
  return position;
}

long Tile::getX() const
{
  return position.x;
}
long Tile::getY() const
{
  return position.y;
}
long Tile::getZ() const
{
  return position.z;
}

bool Tile::isEmpty() const
{
  return !ground && items.empty();
}

bool Tile::allSelected() const
{
  size_t size = items.size();
  if (ground)
    ++size;

  return selectionCount == size;
}

bool Tile::hasSelection() const
{
  return selectionCount != 0;
}
