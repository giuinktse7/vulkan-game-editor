#include "selection.h"

#include "debug.h"
#include "history/history_action.h"
#include "map_view.h"

Selection::Selection(MapView &mapView)
    : outOfBoundCorrection(0, 0, 0),
      mapView(mapView),
      storage(mapView.map()->size())
{
}

void Selection::merge(std::vector<Position> &positions)
{
  for (const auto &pos : positions)
  {
    mapView.getTile(pos)->selectAll();
    storage.add(pos);
  }
}

void Selection::deselect(std::vector<Position> &positions)
{
  for (const auto &pos : positions)
  {
    mapView.getTile(pos)->deselectAll();
    storage.remove(pos);
  }
}

Position Selection::moveDelta() const
{
  if (!moveOrigin || storage.empty())
    return Position(0, 0, 0);

  auto delta = mapView.mouseGamePos() - moveOrigin.value();

  auto topLeftCorrection = storage.getCorner(0, 0, 0).value() + delta;
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

void Selection::clear()
{
  storage.clear();
}

void Selection::deselectAll()
{
  // There is no need to commit an action if there are no selections
  if (storage.empty())
  {
    VME_LOG_D("Storage was empty.");
    return;
  }

  VME_LOG_D("Deselecting.");

  mapView.history.startGroup(ActionGroupType::Selection);
  MapHistory::Action action(MapHistory::ActionType::Selection);

  const auto positions = storage.allPositions();
  VME_LOG_D("Positions in deselectAll: " << positions.size());

  action.addChange(MapHistory::SelectMultiple(std::move(positions), false));
  // storage.clear();

  mapView.history.commit(std::move(action));
  mapView.history.endGroup(ActionGroupType::Selection);
}

size_t Selection::size() const noexcept
{
  return storage.size();
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

SelectionStorageSet::SelectionStorageSet()
    : xMin(0), yMin(0), xMax(0), yMax(0), zMin(0), zMax(0) {}

void SelectionStorageSet::update()
{
  if (staleBoundingBox)
  {
    recomputeBoundingBox();
    staleBoundingBox = false;
  }
}

void SelectionStorageSet::updateBoundingBox(util::Rectangle<Position::value_type> bbox)
{
  yMin = std::min(yMin, bbox.y1);
  xMax = std::max(xMax, bbox.x2);
  yMax = std::max(yMax, bbox.y2);
  xMin = std::min(xMin, bbox.x1);
}

void SelectionStorageSet::updateBoundingBox(Position pos)
{
  xMin = std::min(xMin, pos.x);
  yMin = std::min(yMin, pos.y);
  zMin = std::min(zMin, pos.z);

  xMax = std::max(xMax, pos.x);
  yMax = std::max(yMax, pos.y);
  zMax = std::max(zMax, pos.z);
}

void SelectionStorageSet::setBoundingBox(Position pos)
{
  xMin = pos.x;
  yMin = pos.y;
  zMin = pos.z;

  xMax = pos.x;
  yMax = pos.y;
  zMax = pos.z;
}

void SelectionStorageSet::add(Position pos)
{
  if (values.empty())
    setBoundingBox(pos);
  else
    updateBoundingBox(pos);

  values.emplace(pos);
}

void SelectionStorageSet::remove(Position pos)
{
  if (xMin == pos.x || xMax == pos.x || yMin == pos.y || yMax == pos.y)
    staleBoundingBox = true;

  values.erase(pos);
}

void SelectionStorageSet::add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox)
{
  if (positions.empty())
    return;

  if (values.empty())
    setBoundingBox(positions.front());

  for (const auto &pos : positions)
    values.emplace(pos);

  updateBoundingBox(bbox);
}

void SelectionStorageSet::recomputeBoundingBox()
{
  for (const auto &pos : values)
    updateBoundingBox(pos);
}

void SelectionStorageSet::clear()
{
  values.clear();
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>SelectionStorageOctree>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SelectionStorageOctree::SelectionStorageOctree(const util::Volume<uint16_t, uint16_t, uint8_t> mapSize)
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

size_t SelectionStorageOctree::size() const noexcept
{
  return tree.size();
}

bool SelectionStorageOctree::contains(const Position pos) const
{
  return tree.contains(pos);
}

void SelectionStorageOctree::clear()
{
  tree.clear();
}

const std::vector<Position> SelectionStorageOctree::allPositions() const
{
  std::vector<Position> positions;
  for (const auto &p : tree)
    positions.emplace_back(p);

  DEBUG_ASSERT(positions.size() == size(), "Amount of positions from the iterator should always be equal to the size.");

  return positions;
}