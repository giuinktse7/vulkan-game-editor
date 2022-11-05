#pragma once

#include <optional>
#include <string>
#include <variant>


class Item;

namespace ItemMutation
{
    enum class MutationType
    {
        SubType,
        ActionId
    };

    struct BaseMutation
    {
        BaseMutation(MutationType type);
        virtual void commit(Item *item) = 0;
        virtual void undo(Item *item) = 0;

        MutationType type;
    };

    struct SetSubType : BaseMutation
    {
        SetSubType(uint8_t subtype);
        void commit(Item *item) override;
        void undo(Item *item) override;

        uint8_t subtype;
    };

    struct SetActionId : BaseMutation
    {
        SetActionId(uint16_t actionId);
        void commit(Item *item) override;
        void undo(Item *item) override;

        uint16_t actionId;
    };

    struct SetText : BaseMutation
    {
        SetText(std::optional<std::string> text);
        void commit(Item *item) override;
        void undo(Item *item) override;

        std::optional<std::string> text;
    };

    using Mutation = std::variant<SetSubType, SetActionId, SetText>;
} // namespace ItemMutation