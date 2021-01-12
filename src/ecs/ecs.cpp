#include "ecs.h"

#include "../logger.h"

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
        VME_LOG_D("Recycled entity id: " << id);
        entityIdQueue.pop();
    }

    entityComponentBitsets.emplace(id, ecs::ComponentBitset{});
    return id;
}
