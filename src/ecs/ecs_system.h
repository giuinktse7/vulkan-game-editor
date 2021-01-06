#pragma once

#include <unordered_set>

#include "entity.h"

class ECS;

namespace ecs
{
    class System
    {
      public:
        void clear()
        {
            entities.clear();
        }

        virtual ~System()
        {
            //Empty
        }

      protected:
        friend class ::ECS;

        virtual std::vector<const char *> getRequiredComponents() const = 0;

        ecs::ComponentBitset componentBitset;
        std::unordered_set<EntityId> entities;
    };

} // namespace ecs
