#pragma once

#include <functional>
#include <optional>
#include <unordered_set>

#include "tile.h"
#include "util.h"
#include "octree.h"

class MapView;

class SelectionStorageInterface
{
  virtual void add(Position pos) = 0;
  virtual void add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox) = 0;

  virtual void remove(Position pos) = 0;

  virtual void update() = 0;

  virtual bool empty() const noexcept = 0;

  virtual bool contains(const Position pos) const = 0;

  virtual void clear() = 0;

  virtual const std::unordered_set<Position, PositionHash> &getPositions() const = 0;
};

class SelectionStorageOctree : SelectionStorageInterface
{
  SelectionStorageOctree(const vme::octree::Cube mapSize);

  void add(Position pos);
  void add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox);

  void remove(Position pos);

  std::optional<Position> topLeft() const noexcept;
  std::optional<Position> topRight() const noexcept;
  std::optional<Position> bottomRight() const noexcept;
  std::optional<Position> bottomLeft() const noexcept;

  void update();

  bool empty() const noexcept;

  bool contains(const Position pos) const;

  void clear();

  const std::unordered_set<Position, PositionHash> &getPositions() const;

private:
  vme::octree::Tree tree;
};

class SelectionStorage
{
public:
  SelectionStorage();

  void add(Position pos);
  void add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox);

  void remove(Position pos);

  std::optional<Position> topLeft() const noexcept
  {
    return empty() ? std::optional<Position>{} : Position(xMin, yMin, zMin);
  }

  std::optional<Position> topRight() const noexcept
  {
    return empty() ? std::optional<Position>{} : Position(xMax, yMin, zMin);
  }

  std::optional<Position> bottomRight() const noexcept
  {
    return empty() ? std::optional<Position>{} : Position(xMax, yMax, zMin);
  }

  std::optional<Position> bottomLeft() const noexcept
  {
    return empty() ? std::optional<Position>{} : Position(xMin, yMin, zMin);
  }

  void update();

  bool empty() const noexcept
  {
    return values.empty();
  }

  bool contains(const Position pos) const
  {
    return values.find(pos) != values.end();
  }

  void clear();

  const std::unordered_set<Position, PositionHash> &getPositions() const;

private:
  std::unordered_set<Position, PositionHash> values;

  WorldPosition::value_type xMin, yMin, xMax, yMax;
  int zMin, zMax;

  bool staleBoundingBox = false;

  void setBoundingBox(Position pos);
  void updateBoundingBox(Position pos);
  void updateBoundingBox(util::Rectangle<Position::value_type> bbox);

  void recomputeBoundingBox();
};

class Selection
{
public:
  Selection(MapView &mapView);
  bool blockDeselect = false;
  std::optional<Position> moveOrigin = {};
  /*
    When the mouse goes outside of the map dimensions, this correction is used to
    stop the selection from also going out of bounds.
  */
  Position outOfBoundCorrection;

  // TODO

  bool moving() const;

  Position moveDelta() const;

  bool contains(const Position pos) const;
  void select(const Position pos);
  void setSelected(const Position pos, bool selected);
  void deselect(const Position pos);
  void deselect(std::unordered_set<Position, PositionHash> &positions);
  void merge(std::unordered_set<Position, PositionHash> &positions);

  bool empty() const;

  const std::unordered_set<Position, PositionHash> &getPositions() const;

  void deselectAll();

  void update();

  /*
    Clear the selected tile positions. NOTE: This function does not call deselect
    on the tiles. For that, use deselectAll().
  */
  void clear();

private:
  MapView &mapView;

  SelectionStorage storage;
};
