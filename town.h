#pragma once

#include <map>
#include <string>
#include "position.h"

class Town
{
public:
  Town(uint32_t _id) : id(_id), name("") {}
  Town(const Town &copy) : id(copy.id), name(copy.name), templePosition(copy.templePosition) {}
  ~Town() {}

  const std::string &getName() const { return name; }
  void setName(const std::string &newName) { name = newName; }

  const Position &getTemplePosition() const { return templePosition; }
  void setTemplePosition(const Position &_pos);

  uint32_t getID() const { return id; }
  void setID(uint32_t id) { this->id = id; }

private:
  uint32_t id;
  std::string name;
  Position templePosition;
};

class Towns
{
public:
  Towns();
  ~Towns();

  void clear();
  size_t count() const { return towns.size(); }

  bool addTown(Town &town);
  uint32_t getEmptyID();

  Town *getTown(std::string &townname);
  Town *getTown(uint32_t _townid);

  std::map<uint32_t, Town>::const_iterator begin() const { return towns.begin(); }
  std::map<uint32_t, Town>::const_iterator end() const { return towns.end(); }
  std::map<uint32_t, Town>::const_iterator find(uint32_t id) const { return towns.find(id); }
  std::map<uint32_t, Town>::iterator begin() { return towns.begin(); }
  std::map<uint32_t, Town>::iterator end() { return towns.end(); }
  std::map<uint32_t, Town>::iterator find(uint32_t id) { return towns.find(id); }

private:
  std::map<uint32_t, Town> towns;
};