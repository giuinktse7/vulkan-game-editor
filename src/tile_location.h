#pragma once

#include "tile.h"
#include "position.h"

class Tile;
class Item;

class TileLocation
{
	TileLocation();

public:
	~TileLocation();

	TileLocation(const TileLocation &) = delete;
	TileLocation &operator=(const TileLocation &) = delete;

	std::unique_ptr<Tile> replaceTile(Tile &&tile);

	Tile *tile() const;
	Item *ground() const;
	bool hasTile() const;
	bool hasGround() const;

	void setEmptyTile();
	void removeTile();
	std::unique_ptr<Tile> dropTile();

	friend class Floor;
	friend class Tile;

	void setTile(std::unique_ptr<Tile> tile);

	inline Position position() const
	{
		return _position;
	}

	inline long x() const
	{
		return _position.x;
	}

	inline long y() const
	{
		return _position.y;
	}

	inline long z() const
	{
		return _position.z;
	}

protected:
	std::unique_ptr<Tile> _tile{};
	Position _position;
};