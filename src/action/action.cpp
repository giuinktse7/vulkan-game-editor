#include "action.h"

#include <iterator>

#include "../debug.h"
#include "../position.h"
#include "../map_view.h"
#include "../util.h"

MapAction::MapAction(MapView &mapView, MapActionType actionType)
    : committed(false),
      actionType(actionType),
      mapView(mapView)
{
}

void MapAction::addChange(Change &&change)
{
  changes.push_back(std::move(change));
}

void MapAction::markAsCommitted()
{
  committed = true;
}

void MapAction::commit()
{
  DEBUG_ASSERT(!committed, "Attempted to commit an already committed action.");

  for (auto &change : changes)
  {
    std::visit(util::overloaded{
                   [this, &change](Change::TileData &newTile) {
                     std::unique_ptr<Tile> oldTilePtr = mapView.setTileInternal(std::move(newTile));

                     Tile *oldTile = oldTilePtr.release();
                     // TODO Destroy ECS components for the items of the Tile
                     change.data = std::move(*oldTile);
                   },
                   [this, &change](Change::RemovedTileData &tileChange) {
                     Position &position = std::get<Position>(tileChange.data);
                     std::unique_ptr<Tile> tilePtr = mapView.removeTileInternal(position);
                     change.data = RemovedTile{std::move(*tilePtr.release())};

                     // TODO Destroy ECS components for the items of the Tile
                   },
                   [this](Change::FullSelectionData &data) {
                     if (data.isDeselect)
                     {
                       for (const auto pos : data.positions)
                       {
                         mapView.getTile(pos)->deselectAll();
                       }
                       mapView.selection.deselect(data.positions);
                     }
                     else
                     {
                       for (const auto pos : data.positions)
                       {
                         mapView.getTile(pos)->selectAll();
                       }
                       mapView.selection.merge(data.positions);
                     }
                     data.isDeselect = !data.isDeselect;
                   },
                   [this](Change::SelectionData &data) {
                     Tile *tile = mapView.getTile(data.position);
                     for (const auto i : data.indices)
                     {
                       if (data.select)
                       {
                         tile->selectItemAtIndex(i);
                       }
                       else
                       {
                         tile->deselectItemAtIndex(i);
                       }
                     }
                     if (data.includesGround)
                     {
                       tile->selectGround();
                     }

                     if (tile->hasSelection())
                     {
                       mapView.selection.select(data.position);
                     }
                     else
                     {
                       mapView.selection.deselect(data.position);
                     }

                     data.select = !data.select;
                   },
                   [](auto &) {
                     ABORT_PROGRAM("Unknown change!");
                   }},
               change.data);
  }

  committed = true;
}

void MapAction::undo()
{
  if (changes.empty())
  {
    return;
  }

  for (auto it = changes.rbegin(); it != changes.rend(); ++it)
  {
    Change &change = *it;

    std::visit(util::overloaded{
                   [this, &change](Tile &oldTile) {
                     std::unique_ptr<Tile> newTilePtr = mapView.setTileInternal(std::move(oldTile));
                     Tile *newTile = newTilePtr.release();

                     // TODO Destroy ECS components for the items of the Tile

                     change.data = std::move(*newTile);
                   },

                   [this, &change](RemovedTile &tileChange) {
                     Tile &removedTile = std::get<Tile>(tileChange.data);
                     Position pos = removedTile.getPosition();

                     mapView.setTileInternal(std::move(removedTile));

                     change.data = RemovedTile{pos};
                   },
                   [this](Change::FullSelectionData &data) {
                     if (data.isDeselect)
                     {
                       for (const auto pos : data.positions)
                       {
                         mapView.getTile(pos)->selectAll();
                       }
                       mapView.selection.merge(data.positions);
                     }
                     else
                     {
                       for (const auto pos : data.positions)
                       {
                         mapView.getTile(pos)->deselectAll();
                       }
                       mapView.selection.deselect(data.positions);
                     }

                     data.isDeselect = !data.isDeselect;
                   },
                   [this](Change::SelectionData &data) {
                     Tile *tile = mapView.getTile(data.position);
                     for (const auto i : data.indices)
                     {
                       if (data.select)
                       {
                         tile->selectItemAtIndex(i);
                       }
                       else
                       {
                         tile->deselectItemAtIndex(i);
                       }
                     }
                     if (data.includesGround)
                     {
                       tile->deselectGround();
                     }

                     if (tile->hasSelection())
                     {
                       mapView.selection.select(data.position);
                     }
                     else
                     {
                       mapView.selection.deselect(data.position);
                     }

                     data.select = !data.select;
                   },
                   [](auto &) {
                     ABORT_PROGRAM("Unknown change!");
                   }},
               change.data);
  }

  committed = false;
}
void MapAction::redo()
{
  commit();
}

Change::Change()
    : data({})
{
}

Change::Change(Tile &&tile)
    : data(std::move(tile))
{
}

Change Change::setTile(Tile &&tile)
{
  Change change;
  change.data = std::move(tile);
  return change;
}

Change Change::selection(const Tile &tile)
{
  Change change;
  Change::SelectionData data;
  data.position = tile.getPosition();

  Item *ground = tile.getGround();
  data.includesGround = ground && ground->selected;

  auto &items = tile.getItems();
  for (size_t i = 0; i < items.size(); ++i)
  {
    const Item &item = items.at(i);
    if (item.selected)
    {
      data.indices.emplace_back(static_cast<uint16_t>(i));
    }
  }

  change.data = data;
  return change;
}

Change Change::selection(std::unordered_set<Position, PositionHash> positions)
{
  Change change;
  FullSelectionData data;
  data.positions = positions;
  data.isDeselect = false;

  change.data = data;
  return change;
}

Change Change::deselection(std::unordered_set<Position, PositionHash> positions)
{
  Change change;
  FullSelectionData data;
  data.positions = positions;
  data.isDeselect = true;

  change.data = data;
  return change;
}

Change Change::selectTopItem(Tile &tile)
{
  Change::SelectionData data{};
  data.position = tile.getPosition();
  data.select = true;

  if (tile.getTopItem() == tile.getGround())
  {
    data.includesGround = true;
  }
  else
  {
    data.includesGround = false;
    data.indices.emplace_back(static_cast<uint16_t>(tile.getItemCount() - 1));
  }

  Change change;
  change.data = data;
  return change;
}

Change Change::deselectTopItem(Tile &tile)
{
  Change change = selectTopItem(tile);
  std::get<Change::SelectionData>(change.data).select = false;
  return change;
}

Change Change::removeTile(const Position pos)
{
  Change change;
  change.data = RemovedTile{pos};
  return change;
}

MapActionGroup::MapActionGroup(ActionGroupType groupType) : groupType(groupType)
{
}

void MapActionGroup::addAction(MapAction &&action)
{
  actions.push_back(std::move(action));
}

void MapActionGroup::commit()
{
  for (auto &action : actions)
  {
    if (!action.isCommitted())
    {
      action.commit();
    }
  }
}

void MapActionGroup::undo()
{
  for (auto it = actions.rbegin(); it != actions.rend(); ++it)
  {
    MapAction &action = *it;
    action.undo();
  }
}

MapAction *EditorHistory::getLatestAction()
{
  if (!currentGroup.has_value() || currentGroup.value().actions.empty())
  {
    return nullptr;
  }
  else
  {
    return &currentGroup.value().actions.back();
  }
}

void EditorHistory::commit(MapAction &&action)
{
  DEBUG_ASSERT(currentGroup.has_value(), "There is no current group.");

  if (!action.committed)
  {
    action.commit();
  }

  MapAction *currentAction = getLatestAction();
  if (currentAction && currentAction->getType() == action.getType())
  {
    // Same action type, can merge actions
    util::appendVector(std::move(action.changes), currentAction->changes);
  }
  else
  {
    currentGroup.value().addAction(std::move(action));
  }
}

bool EditorHistory::currentGroupType(ActionGroupType groupType) const
{
  return currentGroup.has_value() && currentGroup.value().groupType == groupType;
}

void EditorHistory::startGroup(ActionGroupType groupType)
{
  DEBUG_ASSERT(currentGroup.has_value() == false, "The previous group was not ended.");
  // VME_LOG_D("EditorHistory::startGroup: " << groupType);

  currentGroup.emplace(groupType);
}

void EditorHistory::endGroup(ActionGroupType groupType)
{
  DEBUG_ASSERT(currentGroup.has_value(), "There is no current group to end.");
  std::ostringstream s;
  s << "Tried to end group type " << groupType << ", but the current group type is " << currentGroup.value().groupType;
  DEBUG_ASSERT(currentGroup.value().groupType == groupType, s.str());
  // VME_LOG_D("endGroup " << groupType);

  actionGroups.emplace(std::move(currentGroup.value()));
  currentGroup.reset();
}

void EditorHistory::undoLast()
{
  if (currentGroup.has_value())
  {
    endGroup(currentGroup.value().groupType);
  }

  if (!actionGroups.empty())
  {
    actionGroups.top().undo();
    actionGroups.pop();
  }
}

bool EditorHistory::hasCurrentGroup() const
{
  return currentGroup.has_value();
}
