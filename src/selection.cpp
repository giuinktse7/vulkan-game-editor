#include "selection.h"

#include "map_view.h"
#include "debug.h"
#include "history/history_action.h"

Selection::Selection(MapView &mapView) : mapView(mapView)
{
}

void Selection::merge(std::unordered_set<Position, PositionHash> &positions)
{
  for (const auto &pos : positions)
  {
    mapView.getTile(pos)->selectAll();
    positionsWithSelection.emplace(pos);
  }
}

void Selection::deselect(std::unordered_set<Position, PositionHash> &positions)
{
  for (const auto &pos : positions)
  {
    mapView.getTile(pos)->deselectAll();
    positionsWithSelection.erase(pos);
  }
}

bool Selection::contains(const Position pos) const
{
  return positionsWithSelection.find(pos) != positionsWithSelection.end();
}

void Selection::select(const Position pos)
{
  DEBUG_ASSERT(mapView.getTile(pos)->hasSelection(), "The tile does not have a selection.");

  positionsWithSelection.emplace(pos);
}

void Selection::deselect(const Position pos)
{
  positionsWithSelection.erase(pos);
}

void Selection::setSelected(const Position pos, bool selected)
{
  if (selected)
    select(pos);
  else
    deselect(pos);
}

std::unordered_set<Position, PositionHash> Selection::getPositions() const
{
  return positionsWithSelection;
}

void Selection::clear()
{
  positionsWithSelection.clear();
}

void Selection::deselectAll()
{
  // There is no need to commit an action if there are no selections
  if (positionsWithSelection.empty())
  {
    return;
  }

  mapView.history.startGroup(ActionGroupType::Selection);
  MapHistory::Action action(MapHistory::ActionType::Selection);

  action.addChange(MapHistory::SelectMultiple(positionsWithSelection, false));
  positionsWithSelection.clear();

  mapView.history.commit(std::move(action));
  mapView.history.endGroup(ActionGroupType::Selection);
}

bool Selection::empty() const
{
  return positionsWithSelection.empty();
}
