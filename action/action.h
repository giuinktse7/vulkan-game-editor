#pragma once

#include <vector>
#include <stack>
#include <unordered_map>

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
  RemoveMapItem
};

enum class MapActionType
{
  Selection,
  ModifyTile,
  SetTile,
  Move,
  RemoveTile,
  CutTile,
  PasteTile
};

class MapAction
{
public:
  MapAction(MapView &mapView, MapActionType actionType);

  MapAction(const MapAction &other) = delete;
  MapAction &operator=(const MapAction &other) = delete;

  MapAction(MapAction &&other) noexcept
      : committed(other.committed),
        actionType(other.actionType),
        mapView(other.mapView),
        changes(std::move(other.changes))
  {
  }

  void addChange(Change &&change);

  void commit();
  void markAsCommitted();

  void undo();
  void redo();

  bool isCommitted() const
  {
    return committed;
  }

  MapActionType getType() const
  {
    return actionType;
  }

private:
  friend class EditorHistory;
  bool committed;

  MapActionType actionType;
  MapView &mapView;
  std::vector<Change> changes;
};

/*
  Represents a group of actions. One undo command will undo all the actions in
  the group.
*/
class MapActionGroup
{
public:
  MapActionGroup(ActionGroupType groupType);
  void addAction(MapAction &&action);

  MapActionGroup(MapActionGroup &&other) noexcept
      : groupType(other.groupType),
        actions(std::move(other.actions))
  {
  }

  void commit();
  void undo();

  ActionGroupType groupType;

private:
  friend class EditorHistory;
  std::vector<MapAction> actions;
};

struct RemovedTile
{
  /* Holds either:
      1. The tile that was deleted.
      2. The position for where a tile should be deleted.
  */
  std::variant<Tile, Position> data;
};

class Change
{
public:
  struct FullSelectionData
  {
    std::unordered_set<Position, PositionHash> positions;
    bool isDeselect;
  };
  using TileData = Tile;
  using RemovedTileData = RemovedTile;

  struct SelectionData
  {
    Position position;
    std::vector<uint16_t> indices;
    bool includesGround;
    bool select = true;
  };

  Change(Tile &&tile);

  static Change removeTile(const Position pos);
  static Change setTile(Tile &&tile);
  static Change selection(std::unordered_set<Position, PositionHash> positions);
  static Change deselection(std::unordered_set<Position, PositionHash> positions);

  static Change selectTopItem(Tile &tile);
  static Change deselectTopItem(Tile &tile);

  static Change selection(const Tile &tile);

  Change(const Change &other) = delete;
  Change &operator=(const Change &other) = delete;

  Change(Change &&other) noexcept
      : data(std::move(other.data))
  {
  }

  Change &operator=(Change &&other)
  {
    data = std::move(other.data);
    return *this;
  }

  /*
    If the change has not yet been committed, this member contains the new data.
    If the change has been committed, this member contains the old data, i.e.
    the data necessary to undo the change.
  */
  std::variant<std::monostate, TileData, RemovedTileData, SelectionData, FullSelectionData> data;

  template <typename T>
  bool ofType() const
  {
    return std::holds_alternative<T>(data);
  }

private:
  Change();
};

class EditorHistory
{
public:
  void commit(MapAction &&action);
  void undoLast();

  void startGroup(ActionGroupType groupType);
  void endGroup(ActionGroupType groupType);

  bool hasCurrentGroup() const;
  bool currentGroupType(ActionGroupType groupType) const;

private:
  std::optional<MapActionGroup> currentGroup;
  std::stack<MapActionGroup> actionGroups;

  MapAction *getLatestAction();
};

inline std::ostream &operator<<(std::ostream &os, ActionGroupType type)
{
  switch (type)
  {
  case ActionGroupType::Selection:
    os << "ActionGroupType::Selection";
    break;
  case ActionGroupType::AddMapItem:
    os << "ActionGroupType::AddMapItem";
    break;
  case ActionGroupType::RemoveMapItem:
    os << "ActionGroupType::RemoveMapItem";
    break;
  default:
    os << "Unknown ActionGroupType: " << to_underlying(type);
    break;
  }

  return os;
}
