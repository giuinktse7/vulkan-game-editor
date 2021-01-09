#pragma once

#include <type_traits>

#include <stack>
#include <unordered_map>
#include <vector>

#include "../ecs/item_animation.h"
#include "../position.h"
#include "../tile_location.h"
#include "history_change.h"

class Change;
class MapView;
class EditorHistory;

enum class TransactionType
{
    RawItemAction,
    Selection,
    AddMapItem,
    RemoveMapItem,
    MoveItems,
    ModifyItem
};

namespace MapHistory
{
    class Action
    {
      public:
        Action(ActionType actionType)
            : actionType(actionType), committed(false) {}
        Action(ActionType actionType, Change::DataTypes &&change);

        Action(const Action &other) = delete;
        Action &operator=(const Action &other) = delete;

        Action(Action &&other) noexcept
            : changes(std::move(other.changes)),
              actionType(other.actionType),
              committed(other.committed) {}

        void reserve(size_t size);
        void shrinkToFit();

        void addChange(Change::DataTypes &&change);

        void commit(MapView &mapView);
        void markAsCommitted();

        void undo(MapView &mapView);
        void redo(MapView &mapView);

        bool isCommitted() const;

        MapHistory::ActionType getType() const;

        template <typename T>
        bool isType(Change &change) const;

        std::vector<Change> changes;

      private:
        friend class MapHistory::History;

        MapHistory::ActionType actionType;
        bool committed;
    };

    /*
  Represents a group of actions. One undo command will undo all the actions in
  the group.
*/
    class Transaction
    {
      public:
        Transaction(TransactionType type);
        void addAction(MapHistory::Action &&action);

        Transaction(Transaction &&other) noexcept;
        Transaction &operator=(Transaction &&other);

        void commit(MapView &mapView);

        void redo(MapView &mapView);
        void undo(MapView &mapView);

        inline bool empty() const noexcept
        {
            return actions.empty();
        }

        TransactionType type;

      private:
        friend class MapHistory::History;
        std::vector<MapHistory::Action> actions;
    };
} // namespace MapHistory

template <typename T>
bool MapHistory::Action::isType(Change &change) const
{
    return dynamic_cast<T *>(&change) != nullptr;
}

inline std::ostringstream stringify(const TransactionType &type)
{
    std::ostringstream s;

    switch (type)
    {
        case TransactionType::Selection:
            s << "TransactionType::Selection";
            break;
        case TransactionType::AddMapItem:
            s << "TransactionType::AddMapItem";
            break;
        case TransactionType::RemoveMapItem:
            s << "TransactionType::RemoveMapItem";
            break;
        case TransactionType::MoveItems:
            s << "TransactionType::MoveItems";
            break;
        case TransactionType::RawItemAction:
            s << "TransactionType::RawItemAction";
            break;
        default:
            s << "Unknown TransactionType: " << to_underlying(type);
            break;
    }

    return s;
}

inline std::ostream &operator<<(std::ostream &os, const TransactionType &type)
{
    os << stringify(type).str();
    return os;
}
