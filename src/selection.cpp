#include "selection.h"

#include "map_view.h"
#include "debug.h"
#include "history/history_action.h"

Selection::Selection(MapView &mapView)
    : outOfBoundCorrection(0, 0, 0), mapView(mapView)
{
}

void Selection::merge(std::unordered_set<Position, PositionHash> &positions)
{
  for (const auto &pos : positions)
  {
    mapView.getTile(pos)->selectAll();
    storage.add(pos);
  }
}

void Selection::deselect(std::unordered_set<Position, PositionHash> &positions)
{
  for (const auto &pos : positions)
  {
    mapView.getTile(pos)->deselectAll();
    storage.remove(pos);
  }
}

Position Selection::moveDelta() const
{
  if (!moveOrigin)
    return Position(0, 0, 0);

  auto delta = mapView.mouseGamePos() - moveOrigin.value();

  auto topLeftCorrection = storage.topLeft().value() + delta;
  delta.x -= std::min(topLeftCorrection.x, 0);
  delta.y -= std::min(topLeftCorrection.y, 0);

  return delta;
}

bool Selection::contains(const Position pos) const
{
  return storage.contains(pos);
}

void Selection::select(const Position pos)
{
  DEBUG_ASSERT(mapView.getTile(pos)->hasSelection(), "The tile does not have a selection.");

  storage.add(pos);
}

void Selection::deselect(const Position pos)
{
  storage.remove(pos);
}

void Selection::setSelected(const Position pos, bool selected)
{
  if (selected)
    select(pos);
  else
    deselect(pos);
}

const std::unordered_set<Position, PositionHash> &Selection::getPositions() const
{
  return storage.getPositions();
}

void Selection::clear()
{
  storage.clear();
}

void Selection::deselectAll()
{
  // There is no need to commit an action if there are no selections
  if (storage.empty())
  {
    return;
  }

  mapView.history.startGroup(ActionGroupType::Selection);
  MapHistory::Action action(MapHistory::ActionType::Selection);

  action.addChange(MapHistory::SelectMultiple(storage.getPositions(), false));
  storage.clear();

  mapView.history.commit(std::move(action));
  mapView.history.endGroup(ActionGroupType::Selection);
}

bool Selection::empty() const
{
  return storage.empty();
}

bool Selection::moving() const
{
  return moveOrigin.has_value() && moveDelta() != Position(0, 0, 0);
}

void Selection::update()
{
  storage.update();
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>SelectionStorage>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SelectionStorage::SelectionStorage()
    : xMin(0), yMin(0), xMax(0), yMax(0), zMin(0), zMax(0) {}

void SelectionStorage::update()
{
  if (staleBoundingBox)
  {
    recomputeBoundingBox();
    staleBoundingBox = false;
  }
}

const std::unordered_set<Position, PositionHash> &SelectionStorage::getPositions() const
{
  return values;
}

void SelectionStorage::updateBoundingBox(util::Rectangle<Position::value_type> bbox)
{
  yMin = std::min(yMin, bbox.y1);
  xMax = std::max(xMax, bbox.x2);
  yMax = std::max(yMax, bbox.y2);
  xMin = std::min(xMin, bbox.x1);
}

void SelectionStorage::updateBoundingBox(Position pos)
{
  xMin = std::min(xMin, pos.x);
  yMin = std::min(yMin, pos.y);
  zMin = std::min(zMin, pos.z);

  xMax = std::max(xMax, pos.x);
  yMax = std::max(yMax, pos.y);
  zMax = std::max(zMax, pos.z);
}

void SelectionStorage::setBoundingBox(Position pos)
{
  xMin = pos.x;
  yMin = pos.y;
  zMin = pos.z;

  xMax = pos.x;
  yMax = pos.y;
  zMax = pos.z;
}

void SelectionStorage::add(Position pos)
{
  if (values.empty())
    setBoundingBox(pos);
  else
    updateBoundingBox(pos);

  values.emplace(pos);
}

void SelectionStorage::remove(Position pos)
{
  if (xMin == pos.x || xMax == pos.x || yMin == pos.y || yMax == pos.y)
    staleBoundingBox = true;

  values.erase(pos);
}

void SelectionStorage::add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox)
{
  if (positions.empty())
    return;

  if (values.empty())
    setBoundingBox(positions.front());

  for (const auto &pos : positions)
    values.emplace(pos);

  updateBoundingBox(bbox);
}

void SelectionStorage::recomputeBoundingBox()
{
  for (const auto &pos : values)
    updateBoundingBox(pos);
}

void SelectionStorage::clear()
{
  values.clear();
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>SelectionStorageOctree>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SelectionStorageOctree::SelectionStorageOctree(const vme::octree::Cube mapSize)
    : tree(vme::octree::Tree::create(mapSize)) {}

void SelectionStorageOctree::add(Position pos)
{
  tree.add(pos);
}

void SelectionStorageOctree::add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox)
{
  for (const auto &pos : positions)
    tree.add(pos);
}

void SelectionStorageOctree::remove(Position pos)
{
  tree.remove(pos);
}

void SelectionStorageOctree::update()
{
  // No-op
}

bool SelectionStorageOctree::empty() const noexcept
{
  return tree.empty();
}

bool SelectionStorageOctree::contains(const Position pos) const
{
  return tree.contains(pos);
}

void SelectionStorageOctree::clear()
{
  tree.clear();
}

const std::unordered_set<Position, PositionHash> &SelectionStorageOctree::getPositions() const
{
}