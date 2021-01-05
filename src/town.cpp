#include "town.h"

#include "tile.h"

Town::Town(uint32_t id)
    : _id(id), _name("") {}

const std::string &Town::name() const noexcept
{
    return _name;
}

uint32_t Town::id() const noexcept
{
    return _id;
}

const Position &Town::templePosition() const noexcept
{
    return _templePosition;
}

void Town::setId(uint32_t id)
{
    _id = id;
}

void Town::setName(const std::string &name)
{
    _name = name;
}

void Town::setTemplePosition(const Position position)
{
    _templePosition = position;
}