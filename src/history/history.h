#pragma once

#include <optional>
#include <vector>

#include "history_action.h"

namespace MapHistory
{
  class History
  {
  public:
    History(MapView &mapView);
    void commit(Action &&action);
    void commit(ActionType actionType, Change::DataTypes &&change);

    /**
     * @return true if anything was undone, and false otherwise.
     */
    bool undo();

    /**
     * @return true if anything was redone, and false otherwise.
     */
    bool redo();

    void beginTransaction(TransactionType transactionType);
    void endTransaction(TransactionType transactionType);

    bool hasCurrentTransaction() const;
    bool hasCurrentTransactionType(TransactionType transactionType) const;

  private:
    std::optional<Transaction> currentTransaction;
    std::vector<Transaction> transactions;
    size_t insertionIndex;

    MapView *mapView;

    Action *getLatestAction();
  };
} // namespace MapHistory
