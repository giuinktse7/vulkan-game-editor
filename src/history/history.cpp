#include "history.h"

#include "../debug.h"

namespace MapHistory
{
  History::History(MapView &mapView) : mapView(&mapView) {}

  Action *History::getLatestAction()
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

  void History::commit(Action &&action)
  {
    DEBUG_ASSERT(currentGroup.has_value(), "There is no current group.");

    if (!action.committed)
    {
      action.commit(*mapView);
    }

    Action *currentAction = getLatestAction();
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

  bool History::currentGroupType(ActionGroupType groupType) const
  {
    return currentGroup.has_value() && currentGroup.value().groupType == groupType;
  }

  void History::startGroup(ActionGroupType groupType)
  {
    DEBUG_ASSERT(currentGroup.has_value() == false, "The previous group was not ended.");
    // VME_LOG_D("History::startGroup: " << groupType);

    currentGroup.emplace(groupType);
  }

  void History::endGroup(ActionGroupType groupType)
  {
    DEBUG_ASSERT(currentGroup.has_value(), "There is no current group to end.");
    std::ostringstream s;
    s << "Tried to end group type " << groupType << ", but the current group type is " << currentGroup.value().groupType;
    DEBUG_ASSERT(currentGroup.value().groupType == groupType, s.str());
    // VME_LOG_D("endGroup " << groupType);

    actionGroups.emplace(std::move(currentGroup.value()));
    currentGroup.reset();
  }

  void History::undoLast()
  {
    if (currentGroup.has_value())
    {
      endGroup(currentGroup.value().groupType);
    }

    if (!actionGroups.empty())
    {
      actionGroups.top().undo(*mapView);
      actionGroups.pop();
    }
  }

  bool History::hasCurrentGroup() const
  {
    return currentGroup.has_value();
  }
} // namespace MapHistory