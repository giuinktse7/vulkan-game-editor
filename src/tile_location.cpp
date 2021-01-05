#include "tile_location.h"

#include <iostream>
#include <memory>

#include "debug.h"
#include "tile.h"

TileLocation::TileLocation()
{
    // std::cout << "TileLocation()" << std::endl;
}

TileLocation::TileLocation(TileLocation &&other) noexcept
    : _tile(std::move(other._tile)), _position(std::move(other._position)) {}

TileLocation &TileLocation::operator=(TileLocation &&other) noexcept
{
    _tile = std::move(other._tile);
    _position = std::move(other._position);

    return *this;
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

void TileLocation::setTile(Tile &&tile)
{
    _tile = std::make_unique<Tile>(std::move(tile));
    _tile->setLocation(*this);
}

Tile *TileLocation::tile() const
{
    return _tile ? _tile.get() : nullptr;
}

bool TileLocation::hasTile() const
{
    return static_cast<bool>(_tile);
}

bool TileLocation::hasGround() const
{
    return _tile && _tile->ground();
}

void TileLocation::setEmptyTile()
{
    setTile(std::make_unique<Tile>(*this));
}

Item *TileLocation::ground() const
{
    if (!_tile)
        return nullptr;

    return _tile->ground();
}

void TileLocation::removeTile()
{
    _tile.reset();
}

std::unique_ptr<Tile> TileLocation::dropTile()
{
    return std::unique_ptr<Tile>(std::move(_tile));
}

std::unique_ptr<Tile> TileLocation::replaceTile(Tile &&newTile)
{
    DEBUG_ASSERT(!_tile || (newTile.position() == _tile->position()), "The new tile must have the same position as the old tile.");

    auto tile = std::make_unique<Tile>(std::move(newTile));
    _tile.swap(tile);

    return tile;
}

void TileLocation::swapTile(std::unique_ptr<Tile> &&tile)
{
    DEBUG_ASSERT(tile->position() == _position, "The tile must have the same position as the tile location.");
    _tile.swap(std::move(tile));
}
