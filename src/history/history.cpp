#include "history.h"

#include "../debug.h"
#include "../map_view.h"
#include "../util.h"

namespace
{
  constexpr size_t TransactionsReserveAmount = util::power(2, 5);
};

namespace MapHistory
{
  History::History(MapView &mapView)
      : mapView(&mapView), insertionIndex(0)
  {
    transactions.reserve(TransactionsReserveAmount);
  }

  Action *History::getLatestAction()
  {
    if (!currentTransaction.has_value() || currentTransaction.value().actions.empty())
    {
      return nullptr;
    }
    else
    {
      return &currentTransaction.value().actions.back();
    }
  }

  void History::commit(MapHistory::ActionType actionType, MapHistory::Change::DataTypes &&change)
  {
    commit(MapHistory::Action(actionType, std::move(change)));
  }

  void History::commit(Action &&action)
  {
    DEBUG_ASSERT(currentTransaction.has_value(), "There is no current transaction.");

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
      currentTransaction.value().addAction(std::move(action));
    }
  }

  bool History::hasCurrentTransactionType(TransactionType type) const
  {
    return currentTransaction.has_value() && currentTransaction.value().type == type;
  }

  void History::beginTransaction(TransactionType type)
  {
    DEBUG_ASSERT(currentTransaction.has_value() == false, "The previous transaction was not ended.");
    // VME_LOG_D("History::beginTransaction: " << type);

    currentTransaction.emplace(type);
  }

  void History::endTransaction(TransactionType type)
  {
    // VME_LOG_D("History::endTransaction: " << type);
    DEBUG_ASSERT(currentTransaction.has_value(), "There is no current transaction to end.");
    DEBUG_ASSERT(currentTransaction.value().type == type, "The current transaction type differs from the passed in transaction type.");

    if (!currentTransaction.value().empty())
    {
      if (insertionIndex < transactions.size())
      {
        transactions.erase(transactions.begin() + insertionIndex, transactions.end());
      }

      transactions.emplace_back(std::move(currentTransaction.value()));
      ++insertionIndex;
    }

    currentTransaction.reset();

    mapView->selection().update();
  }

  bool History::undo()
  {
    if (currentTransaction.has_value())
    {
      endTransaction(currentTransaction.value().type);
    }

    if (insertionIndex == 0)
    {
      return false;
    }
    else
    {
      transactions.at(insertionIndex - 1).undo(*mapView);
      --insertionIndex;

      return true;
    }
  }

  bool History::redo()
  {
    if (insertionIndex == transactions.size())
      return false;

    transactions.at(insertionIndex).redo(*mapView);
    ++insertionIndex;

    return true;
  }

  bool History::hasCurrentTransaction() const
  {
    return currentTransaction.has_value();
  }
} // namespace MapHistory
