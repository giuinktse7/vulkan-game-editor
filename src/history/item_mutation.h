#pragma once

#include <variant>

class Item;

namespace ItemMutation
{
    enum class MutationType
    {
        Count
    };

    struct BaseMutation
    {
        BaseMutation(MutationType type);
        virtual void commit(Item *item) = 0;
        virtual void undo(Item *item) = 0;

        MutationType type;
    };

    struct SetCount : BaseMutation
    {
        SetCount(uint8_t count);
        void commit(Item *item) override;
        void undo(Item *item) override;

        uint8_t count;
    };

    using Mutation = std::variant<SetCount>;
} // namespace ItemMutation