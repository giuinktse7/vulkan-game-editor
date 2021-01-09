#include "item_mutation.h"

#include "../item.h"

using SetCount = ItemMutation::SetCount;
using MutationType = ItemMutation::MutationType;
using BaseMutation = ItemMutation::BaseMutation;

BaseMutation::BaseMutation(MutationType type)
    : type(type) {}

SetCount::SetCount(uint8_t count)
    : BaseMutation(MutationType::Count), count(count) {}

void SetCount::commit(Item *item)
{
    uint8_t temp = item->count();
    item->setCount(count);
    count = temp;
}

void SetCount::undo(Item *item)
{
    uint8_t temp = item->count();
    item->setCount(count);
    count = temp;
}