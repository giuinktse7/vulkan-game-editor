#pragma once

#include <functional>
#include <optional>
#include <unordered_set>

#include "octree.h"
#include "signal.h"
#include "tile.h"
#include "util.h"

class Map;

class SelectionStorage
{
public:
  virtual bool add(Position pos) = 0;
  virtual bool add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox) = 0;

  virtual bool remove(Position pos) = 0;

  virtual void update() = 0;

  virtual bool empty() const noexcept = 0;

  virtual bool contains(const Position pos) const = 0;

  virtual bool clear() = 0;
  virtual size_t size() const noexcept = 0;

  virtual std::optional<Position> getCorner(bool positiveX, bool positiveY, bool positiveZ) const noexcept = 0;
  virtual std::optional<Position> getCorner(int positiveX, int positiveY, int positiveZ) const noexcept = 0;

  virtual std::vector<Position> allPositions() const = 0;
  virtual std::optional<Position> onlyPosition() const = 0;
};

class SelectionStorageOctree : public SelectionStorage
{
public:
  SelectionStorageOctree(const util::Volume<uint16_t, uint16_t, uint8_t> mapSize);

  bool add(Position pos) override;
  bool add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox) override;

  bool remove(Position pos) override;

  void update() override;

  bool empty() const noexcept override;

  bool contains(const Position pos) const override;

  bool clear() override;

  std::optional<Position> getCorner(bool positiveX, bool positiveY, bool positiveZ) const noexcept override;
  std::optional<Position> getCorner(int positiveX, int positiveY, int positiveZ) const noexcept override;

  vme::octree::Tree::iterator begin()
  {
    return tree.begin();
  }
  vme::octree::Tree::iterator end()
  {
    return tree.end();
  }

  size_t size() const noexcept override;

  std::vector<Position> allPositions() const override;
  std::optional<Position> onlyPosition() const override;

private:
  vme::octree::Tree tree;
};

inline std::optional<Position> SelectionStorageOctree::getCorner(bool positiveX, bool positiveY, bool positiveZ) const noexcept
{
  return tree.getCorner(positiveX, positiveY, positiveZ);
}
inline std::optional<Position> SelectionStorageOctree::getCorner(int positiveX, int positiveY, int positiveZ) const noexcept
{
  return tree.getCorner(positiveX == 1, positiveY == 1, positiveZ == 1);
}

class SelectionStorageSet : public SelectionStorage
{
public:
  SelectionStorageSet();

  bool add(Position pos) override;
  bool add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox) override;

  bool remove(Position pos) override;

  void update() override;

  bool empty() const noexcept override
  {
    return values.empty();
  }

  bool contains(const Position pos) const override
  {
    return values.find(pos) != values.end();
  }

  bool clear() override;

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
  Selection(MapView &mapView, Map &map);
  bool blockDeselect = false;
  std::optional<Position> moveOrigin = {};
  std::optional<Position> moveDelta = {};

  void startMove(const Position &origin);
  void endMove();

  /*
    When the mouse goes outside of the map dimensions, this correction is used to
    stop the selection from also going out of bounds.
  */
  Position outOfBoundCorrection;

  // TODO

  bool isMoving() const;

  vme::octree::Tree::iterator begin()
  {
    return storage.begin();
  }
  vme::octree::Tree::iterator end()
  {
    return storage.end();
  }

  void updateMoveDelta(const Position &currentPosition);

  std::optional<Position> getCorner(bool positiveX, bool positiveY, bool positiveZ) const noexcept;
  std::optional<Position> getCorner(int positiveX, int positiveY, int positiveZ) const noexcept;

  bool contains(const Position pos) const;

  void select(const Position pos);
  void select(const std::vector<Position> &positions);
  void deselect(const Position pos);
  void deselect(const std::vector<Position> &positions);
  void setSelected(const Position pos, bool selected);
  // bool deselectAll();

  inline std::vector<Position> allPositions() const;
  inline std::optional<Position> onlyPosition() const;

  size_t size() const noexcept;

  bool empty() const;

  void update();

  /*
    Clear the selected tile positions. NOTE: This function does not call deselect
    on the tiles. For that, use deselectAll().
  */
  void clear();

  template <auto mem_ptr, typename T>
  void onChanged(T *instance);

private:
  Nano::Signal<void()> selectionChange;

  /**
   * IMPORTANT: Any method that changes the selection *must* set this to true 
   * if the selection was changed. 
   * 
   * This is necessary for selectionChangedEvent to fire when comitting changes 
   * to the map.
   * 
   * @see MapHistory::Transaction
   */
  bool _changed = false;

  Map &map;
  MapView &mapView;

  SelectionStorageOctree storage;
};

inline std::vector<Position> Selection::allPositions() const
{
  return storage.allPositions();
}

inline std::optional<Position> Selection::onlyPosition() const
{
  return storage.onlyPosition();
}

template <auto mem_ptr, typename T>
inline void Selection::onChanged(T *instance)
{
  selectionChange.connect<mem_ptr>(instance);
}