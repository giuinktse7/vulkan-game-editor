#include "history_action.h"

#include <iterator>

#include "../debug.h"
#include "../map_view.h"
#include "../position.h"
#include "../util.h"

namespace MapHistory
{
  ActionGroup::ActionGroup(ActionGroupType groupType) : groupType(groupType) {}

  void ActionGroup::addAction(MapHistory::Action &&action)
  {
    actions.push_back(std::move(action));
  }

  void ActionGroup::commit(MapView &mapView)
  {
    for (auto &action : actions)
    {
      DEBUG_ASSERT(!action.isCommitted(), "An action in an uncommitted ActionGroup should never be committed.");
      action.commit(mapView);
    }

    mapView.selection().update();
  }

  void ActionGroup::undo(MapView &mapView)
  {
    for (auto it = actions.rbegin(); it != actions.rend(); ++it)
    {
      auto &action = *it;
      action.undo(mapView);
    }

    mapView.selection().update();
  }

  Action::Action(ActionType actionType, Change::DataTypes &&change)
      : actionType(actionType), committed(false)
  {
    addChange(std::move(change));
  }

  void Action::redo(MapView &mapView)
  {
    commit(mapView);
  }

  void Action::commit(MapView &mapView)
  {
    DEBUG_ASSERT(!committed, "Attempted to commit an already committed action.");

    for (auto &change : changes)
      change.commit(mapView);

    committed = true;
  }

  void Action::undo(MapView &mapView)
  {
    DEBUG_ASSERT(committed, "Attempted to undo an action that is not committed.");

    for (auto &change : changes)
      change.undo(mapView);

    committed = false;
  }
} // namespace MapHistory
