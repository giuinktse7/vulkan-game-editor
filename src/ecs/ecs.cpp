#include "ecs.h"

#include "../logger.h"
#include "item_animation.h"

ECS g_ecs;

ecs::EntityId ECS::createEntity()
{
    uint32_t id;
    if (entityIdQueue.empty())
    {
        id = entityCounter++;
    }
    else
    {
        id = entityIdQueue.front();
        entityIdQueue.pop();
    }

    entityComponentBitsets.emplace(id, ecs::ComponentBitset{});
    return id;
}
