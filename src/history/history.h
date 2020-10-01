#pragma once

#include "history_action.h"

namespace MapHistory
{
  class History
  {
  public:
    History(MapView &mapView);
    void commit(MapHistory::Action &&action);
    void undoLast();

    void startGroup(ActionGroupType groupType);
    void endGroup(ActionGroupType groupType);

    bool hasCurrentGroup() const;
    bool currentGroupType(ActionGroupType groupType) const;

  private:
    std::optional<MapHistory::ActionGroup> currentGroup;
    std::stack<MapHistory::ActionGroup> actionGroups;

    MapView *mapView;

    MapHistory::Action *getLatestAction();
  };
} // namespace MapHistory
