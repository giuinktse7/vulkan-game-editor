#pragma once

#include "history_action.h"

namespace MapHistory
{
  class History
  {
  public:
    History(MapView &mapView);
    void commit(Action &&action);
    void commit(ActionType actionType, Change::DataTypes &&change);

    void undoLast();

    void startGroup(ActionGroupType groupType);
    void endGroup(ActionGroupType groupType);

    bool hasCurrentGroup() const;
    bool currentGroupType(ActionGroupType groupType) const;

  private:
    std::optional<ActionGroup> currentGroup;
    std::stack<ActionGroup> actionGroups;

    MapView *mapView;

    Action *getLatestAction();
  };
} // namespace MapHistory
