#pragma once

#include <map>
#include <string>

#include "position.h"

class Town
{
  public:
    Town(uint32_t id);

    uint32_t id() const noexcept;
    const std::string &name() const noexcept;
    const Position &templePosition() const noexcept;

    void setId(uint32_t id);
    void setName(const std::string &name);
    void setTemplePosition(const Position position);

  private:
    uint32_t _id;
    std::string _name;
    Position _templePosition;
};
