#pragma once

#include <bitset>
#include <optional>

constexpr size_t MaxEntityComponents = 32;

namespace ecs
{
    using ComponentBitset = std::bitset<MaxEntityComponents>;

    using EntityId = uint32_t;

    class Entity
    {
      public:
        virtual ~Entity()
        {
            destroyEntity();
        }

        template <typename T>
        bool hasComponent() const;
        template <typename T>
        T *getComponent() const;
        template <typename T>
        T *addComponent(T component) const;

        EntityId assignNewEntityId();
        void markForDestruction();

        virtual std::optional<EntityId> getEntityId() const = 0;

        virtual bool isEntity() const = 0;

        virtual void setEntityId(EntityId id) = 0;

        virtual void destroyEntity() {}
    };

    class OptionalEntity : public Entity
    {
      public:
        std::optional<ecs::EntityId> getEntityId() const override;
        bool isEntity() const override;

        void setEntityId(ecs::EntityId id) override;

        void destroyEntity() override;

        std::optional<EntityId> entityId;
    };
} // namespace ecs
