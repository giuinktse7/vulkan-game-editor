#include "selection.h"

#include "debug.h"
#include "history/history_action.h"
#include "map_view.h"

Selection::Selection(MapView &mapView, Map &map)
    : map(map),
      mapView(mapView),
      storage(map.size())
{
  DEBUG_ASSERT(mapView.map() == &map, "The selection map differs from the mapView map.");
}

void Selection::select(const std::vector<Position> &positions)
{
  for (const auto &pos : positions)
  {
    bool change = storage.add(pos);
    _changed = _changed || change;
  }
}

void Selection::deselect(const std::vector<Position> &positions)
{
  for (const auto &pos : positions)
  {
    bool change = storage.remove(pos);
    _changed = _changed || change;
  }
}

bool Selection::isMoving() const noexcept
{
  auto select = mapView.editorAction.as<MouseAction::Select>();
  return select && select->isMoving();
}

bool Selection::contains(const Position pos) const
{
  return storage.contains(pos);
}

void Selection::select(const Position pos)
{
  DEBUG_ASSERT(mapView.getTile(pos)->hasSelection(), "The tile does not have a selection.");

  bool change = storage.add(pos);
  _changed = _changed || change;
}

void Selection::deselect(const Position pos)
{
  bool change = storage.remove(pos);
  _changed = _changed || change;
}

void Selection::setSelected(const Position pos, bool selected)
{
  if (selected)
    select(pos);
  else
    deselect(pos);
}

void Selection::updatePosition(const Position pos)
{
  auto tile = mapView.getTile(pos);
  setSelected(pos, tile && tile->hasSelection());
}

void Selection::clear()
{
  bool change = storage.clear();
  _changed = _changed || change;
}

// bool Selection::deselectAll()
// {
//   // There is no need to commit an action if there are no selections
//   if (storage.empty())
//   {
//     VME_LOG_D("[Selection::deselectAll] Storage was empty.");
//     return false;
//   }

//   deselect(std::move(allPositions()));

//   return true;
// }

size_t Selection::size() const noexcept
{
  return storage.size();
}

bool Selection::empty() const
{
  return storage.empty();
}

void Selection::update()
{
  // storage.update();

  if (_changed)
  {
    selectionChange.fire();
    _changed = false;
  }
}

std::optional<Position> Selection::getCorner(bool positiveX, bool positiveY, bool positiveZ) const noexcept
{
  return storage.getCorner(positiveX, positiveY, positiveZ);
}
std::optional<Position> Selection::getCorner(int positiveX, int positiveY, int positiveZ) const noexcept
{
  return storage.getCorner(positiveX, positiveY, positiveZ);
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

bool SelectionStorageSet::add(Position pos)
{
  if (values.empty())
    setBoundingBox(pos);
  else
    updateBoundingBox(pos);

  values.emplace(pos);

  return true;
}

bool SelectionStorageSet::remove(Position pos)
{
  if (xMin == pos.x || xMax == pos.x || yMin == pos.y || yMax == pos.y)
    staleBoundingBox = true;

  values.erase(pos);

  return true;
}

bool SelectionStorageSet::add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox)
{
  if (positions.empty())
    return false;

  if (values.empty())
    setBoundingBox(positions.front());

  for (const auto &pos : positions)
    values.emplace(pos);

  updateBoundingBox(bbox);

  return true;
}

void SelectionStorageSet::recomputeBoundingBox()
{
  for (const auto &pos : values)
    updateBoundingBox(pos);
}

bool SelectionStorageSet::clear()
{
  values.clear();

  return true;
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>SelectionStorageOctree>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

SelectionStorageOctree::SelectionStorageOctree(const util::Volume<uint16_t, uint16_t, uint8_t> mapSize)
    : tree(vme::octree::Tree::create(mapSize)) {}

bool SelectionStorageOctree::add(Position pos)
{
  return tree.add(pos);
}

bool SelectionStorageOctree::add(std::vector<Position> positions, util::Rectangle<Position::value_type> bbox)
{
  bool changed = false;
  for (const auto &pos : positions)
    changed = changed || tree.add(pos);

  return changed;
}

bool SelectionStorageOctree::remove(Position pos)
{
  return tree.remove(pos);
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

bool SelectionStorageOctree::clear()
{
  return tree.clear();
}

std::vector<Position> SelectionStorageOctree::allPositions() const
{
  std::vector<Position> positions;
  for (const auto &p : tree)
    positions.emplace_back(p);

  DEBUG_ASSERT(positions.size() == size(), "Amount of positions from the iterator should always be equal to the size.");

  return positions;
}

std::optional<Position> SelectionStorageOctree::onlyPosition() const
{
  return tree.onlyPosition();
}
