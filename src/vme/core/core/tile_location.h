#pragma once

#include "position.h"
#include "signal.h"
#include "tile.h"

class Tile;
class Item;

class TileLocation
{
  public:
    TileLocation();
    ~TileLocation();

    TileLocation(const TileLocation &) = delete;
    TileLocation &operator=(const TileLocation &) = delete;

    TileLocation(TileLocation &&other) noexcept;
    TileLocation &operator=(TileLocation &&other) noexcept;

    std::unique_ptr<Tile> replaceTile(Tile &&tile);
    void swapTile(std::unique_ptr<Tile> &tile);

    Tile *tile() const;
    Item *ground() const;
    bool hasTile() const;
    bool hasGround() const;

    void setEmptyTile();
    void removeTile();
    std::unique_ptr<Tile> dropTile();

    friend class Floor;
    friend class Tile;

    void setTile(Tile &&tile);
    void setTile(std::unique_ptr<Tile> tile);

    inline Position position() const
    {
        return _position;
    }

    inline Position::value_type x() const
    {
        return _position.x;
    }

    inline Position::value_type y() const
    {
        return _position.y;
    }

    inline Position::value_type z() const
    {
        return _position.z;
    }

  protected:
    std::unique_ptr<Tile> _tile{};
    Position _position;
};