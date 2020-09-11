#include "tile_location.h"

#include <memory>
#include <iostream>
#include "tile.h"

#include "debug.h"

TileLocation::TileLocation()
{
  // std::cout << "TileLocation()" << std::endl;
}

TileLocation::~TileLocation()
{
  // std::cout << "~TileLocation()" << std::endl;
}

void TileLocation::setTile(std::unique_ptr<Tile> tile)
{
  this->tile = std::move(tile);
  this->tile->setLocation(*this);
}
Tile *TileLocation::getTile() const
{
  return tile ? tile.get() : nullptr;
}

bool TileLocation::hasTile() const
{
  if (tile)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool TileLocation::hasGround() const
{
  return tile && tile->getGround();
}

void TileLocation::setEmptyTile()
{
  setTile(std::make_unique<Tile>(*this));
}

Item *TileLocation::getGround() const
{
  if (!tile)
    return nullptr;

  return tile->getGround();
}

void TileLocation::removeTile()
{
  this->tile.reset();
}

std::unique_ptr<Tile> TileLocation::dropTile()
{
  std::unique_ptr<Tile> result = std::move(this->tile);
  this->tile.reset();
  return result;
}

std::unique_ptr<Tile> TileLocation::replaceTile(Tile &&newTile)
{
  DEBUG_ASSERT(!this->tile || (newTile.getPosition() == this->tile->getPosition()), "The new tile must have the same position as the old tile.");

  std::unique_ptr<Tile> old = std::move(this->tile);
  this->tile = std::make_unique<Tile>(std::move(newTile));
  return old;
}

const Position TileLocation::getPosition() const
{
  return position;
}

long TileLocation::getX() const
{
  return position.x;
}
long TileLocation::getY() const
{
  return position.y;
}
long TileLocation::getZ() const
{
  return position.z;
}