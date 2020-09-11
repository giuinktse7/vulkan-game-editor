#include "ecs.h"

#include "item_animation.h"
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
    entityIdQueue.pop();
  }

  entityComponentBitsets.emplace(id, ecs::ComponentBitset{});
  return id;
}

ecs::EntityId ecs::Entity::assignNewEntityId()
{
  auto id = getEntityId();

  if (id.has_value())
  {
    ABORT_PROGRAM("Should this be allowed? Probably not.");
    // g_ecs.destroy(id.value());
  }

  ecs::EntityId newId = g_ecs.createEntity();
  setEntityId(newId);

  return newId;
}

void ecs::Entity::markForDestruction()
{
  if (isEntity())
  {
    g_ecs.markEntityForDestruction(getEntityId().value());
  }
}

std::optional<ecs::EntityId> ecs::OptionalEntity::getEntityId() const
{
  return entityId;
}

bool ecs::OptionalEntity::isEntity() const
{
  return entityId.has_value();
}

void ecs::OptionalEntity::setEntityId(ecs::EntityId id)
{
  this->entityId = id;
}

void ecs::OptionalEntity::destroyEntity()
{
  if (isEntity() && !g_ecs.isMarkedForDestruction(getEntityId().value()))
  {
    g_ecs.destroy(getEntityId().value());
  }

  entityId.reset();
}
