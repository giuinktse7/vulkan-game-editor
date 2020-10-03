#pragma once

#include <type_traits>

#include <vector>
#include <stack>
#include <unordered_map>

#include "history_change.h"
#include "../tile_location.h"
#include "../ecs/item_animation.h"
#include "../position.h"

class Change;
class MapView;
class EditorHistory;

enum class ActionGroupType
{
  Selection,
  AddMapItem,
  RemoveMapItem,
  MoveItems
};

namespace MapHistory
{
  class Action
  {
  public:
    Action(MapHistory::ActionType actionType) : actionType(actionType), committed(false) {}

    Action(const Action &other) = delete;
    Action &operator=(const Action &other) = delete;

    Action(Action &&other) noexcept
        : changes(std::move(other.changes)),
          actionType(other.actionType),
          committed(other.committed) {}

    std::vector<Change> changes;

    void addChange(Change::DataTypes change)
    {
      changes.emplace_back(std::move(change));
    }

    void commit(MapView &mapView);
    void markAsCommitted();

    void undo(MapView &mapView);
    void redo(MapView &mapView);

    bool isCommitted() const
    {
      return committed;
    }

    MapHistory::ActionType getType() const
    {
      return actionType;
    }

    template <typename T>
    bool ofType() const
    {
      return std::holds_alternative<T>(data);
    }

    template <typename T>
    bool isType(Change &change) const
    {
      return dynamic_cast<T *>(&change) != nullptr;
    }

  private:
    friend class MapHistory::History;

    MapHistory::ActionType actionType;
    bool committed;
  };

  /*
  Represents a group of actions. One undo command will undo all the actions in
  the group.
*/
  class ActionGroup
  {
  public:
    ActionGroup(ActionGroupType groupType);
    void addAction(MapHistory::Action &&action);

    ActionGroup(ActionGroup &&other) noexcept
        : groupType(other.groupType),
          actions(std::move(other.actions))
    {
    }

    void commit(MapView &mapView);
    void undo(MapView &mapView);

    inline bool empty() const noexcept
    {
      return actions.empty();
    }

    ActionGroupType groupType;

  private:
    friend class MapHistory::History;
    std::vector<MapHistory::Action> actions;
  };
} // namespace MapHistory

inline std::ostringstream stringify(const ActionGroupType &type)
{
  std::ostringstream s;

  switch (type)
  {
  case ActionGroupType::Selection:
    s << "ActionGroupType::Selection";
    break;
  case ActionGroupType::AddMapItem:
    s << "ActionGroupType::AddMapItem";
    break;
  case ActionGroupType::RemoveMapItem:
    s << "ActionGroupType::RemoveMapItem";
    break;
  case ActionGroupType::MoveItems:
    s << "ActionGroupType::MoveItems";
    break;
  default:
    s << "Unknown ActionGroupType: " << to_underlying(type);
    break;
  }

  return s;
}

inline std::ostream &operator<<(std::ostream &os, const ActionGroupType &type)
{
  os << stringify(type).str();
  return os;
}
