#include "item_mutation.h"

#include "../item.h"

using SetSubType = ItemMutation::SetSubType;
using SetActionId = ItemMutation::SetActionId;
using SetText = ItemMutation::SetText;
using MutationType = ItemMutation::MutationType;
using BaseMutation = ItemMutation::BaseMutation;

BaseMutation::BaseMutation(MutationType type)
    : type(type) {}

SetSubType::SetSubType(uint8_t subtype)
    : BaseMutation(MutationType::SubType), subtype(subtype) {}

void SetSubType::commit(Item *item)
{
    uint8_t temp = item->count();
    item->setSubtype(subtype);
    subtype = temp;
}

void SetSubType::undo(Item *item)
{
    uint8_t temp = item->subtype();
    item->setSubtype(subtype);
    subtype = temp;
}

SetActionId::SetActionId(uint16_t actionId)
    : BaseMutation(MutationType::ActionId), actionId(actionId) {}

void SetActionId::commit(Item *item)
{
    uint16_t temp = item->actionId();
    item->setActionId(actionId);
    actionId = temp;
}

void SetActionId::undo(Item *item)
{
    uint16_t temp = item->actionId();
    item->setActionId(actionId);
    actionId = temp;
}

SetText::SetText(std::optional<std::string> text)
    : BaseMutation(MutationType::ActionId), text(text) {}

void SetText::commit(Item *item)
{
    std::optional<std::string> temp = item->text();
    if (text.has_value())
    {
        item->setText(*text);
    }
    else
    {
        item->clearText();
    }

    text = temp;
}

void SetText::undo(Item *item)
{
    std::optional<std::string> temp = item->text();
    if (text.has_value())
    {
        item->setText(*text);
    }
    else
    {
        item->clearText();
    }

    text = temp;
}