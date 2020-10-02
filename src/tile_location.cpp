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
  _tile = std::move(tile);
  _tile->setLocation(*this);
}
Tile *TileLocation::tile() const
{
  return _tile ? _tile.get() : nullptr;
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
  return _tile && _tile->getGround();
}

void TileLocation::setEmptyTile()
{
  setTile(std::make_unique<Tile>(*this));
}

Item *TileLocation::getGround() const
{
  if (!_tile)
    return nullptr;

  return _tile->getGround();
}

void TileLocation::removeTile()
{
  _tile.reset();
}

std::unique_ptr<Tile> TileLocation::dropTile()
{
  std::unique_ptr<Tile> result = std::move(_tile);
  _tile.reset();
  return result;
}

std::unique_ptr<Tile> TileLocation::replaceTile(Tile &&newTile)
{
  DEBUG_ASSERT(!_tile || (newTile.position() == _tile->position()), "The new tile must have the same position as the old tile.");

  std::unique_ptr<Tile> old = std::move(_tile);
  _tile = std::make_unique<Tile>(std::move(newTile));
  return old;
}