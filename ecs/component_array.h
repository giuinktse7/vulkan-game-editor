#pragma once

#include <unordered_map>
#include <vector>

#include "entity.h"
#include "../debug.h"
#include "../logger.h"

#include <type_traits>

constexpr size_t InitialCapacity = 32;

class IComponentArray
{
public:
	virtual void entityDestroyed(ecs::EntityId entity) = 0;
	virtual ~IComponentArray() = default;
};

template <typename T>
class ComponentArray : public IComponentArray
{
	using EntityId = uint32_t;

public:
	ComponentArray()
	{
		components.reserve(InitialCapacity);
		entityToComponentIndex.reserve(InitialCapacity);
		componentIndexToEntity.reserve(InitialCapacity);
	}
	~ComponentArray() override
	{
		Logger::debug("~ComponentArray()");
	}

	void addComponent(ecs::EntityId entity, T &component);
	void addComponent(ecs::EntityId entity, T &&component);

	void clear()
	{
		components.clear();
		entityToComponentIndex.clear();
		componentIndexToEntity.clear();
	}

	void removeComponent(ecs::EntityId entity)
	{
		// if (entityToComponentIndex.find(entity) != entityToComponentIndex.end())
		// {
		// 	if (components.size() == 1)
		// 	{
		// 		clear();
		// 	}
		// 	else
		// 	{
		// 		size_t lastIndex = components.size() - 1;
		// 		Entity entityToBeMoved = componentIndexToEntity.at(lastIndex);

		// 		size_t removedIndex = entityToComponentIndex[entity];
		// 		if (std::is_move_assignable<T>::value && std::is_move_constructible<T>::value)
		// 		{
		// 			components.at(removedIndex) = std::move(components.back());
		// 		}
		// 		else
		// 		{
		// 			components.at(removedIndex) = components.back();
		// 		}

		// 		components.pop_back();
		// 		entityToComponentIndex.erase(entity);
		// 		entityToComponentIndex.at(entityToBeMoved) = removedIndex;

		// 		componentIndexToEntity.erase(lastIndex);
		// 		componentIndexToEntity.at(removedIndex) = entityToBeMoved;
		// 	}
		// }

		DEBUG_ASSERT(entityToComponentIndex.find(entity) != entityToComponentIndex.end(), "Removing non-existent component.");

		size_t removedIndex = entityToComponentIndex.at(entity);
		size_t lastIndex = components.size() - 1;

		components.at(removedIndex) = components.at(lastIndex);

		ecs::EntityId lastIndexEntity = componentIndexToEntity.at(lastIndex);
		entityToComponentIndex.at(lastIndexEntity) = removedIndex;
		componentIndexToEntity.at(removedIndex) = lastIndexEntity;

		entityToComponentIndex.erase(entity);
		componentIndexToEntity.erase(lastIndex);
		components.pop_back();
	}

	void entityDestroyed(ecs::EntityId entity) override
	{
		if (entityToComponentIndex.find(entity) != entityToComponentIndex.end())
		{
			removeComponent(entity);
		}
	}

	bool hasComponent(ecs::EntityId entity) const
	{
		return entityToComponentIndex.find(entity) != entityToComponentIndex.end();
	}

	T *getComponent(ecs::EntityId entity)
	{
		// DEBUG_ASSERT(entityIndex.find(entity) != entityIndex.end(), "Entity " + std::to_string(entity.id) + " does not have a " + std::string(typeid(T).name()) + " component.");

		return &components.at(entityToComponentIndex.at(entity));
	}

private:
	std::vector<T> components;
	std::unordered_map<ecs::EntityId, size_t> entityToComponentIndex;
	std::unordered_map<size_t, ecs::EntityId> componentIndexToEntity;
};

template <typename T>
inline void ComponentArray<T>::addComponent(ecs::EntityId entity, T &component)
{
	DEBUG_ASSERT(entityToComponentIndex.find(entity) == entityToComponentIndex.end(), "The entity " + std::to_string(entity) + " already has a component of type " + typeid(T).name());

	components.push_back(component);
	entityToComponentIndex.emplace(entity, components.size() - 1);
	componentIndexToEntity.emplace(components.size() - 1, entity);
}

template <typename T>
inline void ComponentArray<T>::addComponent(ecs::EntityId entity, T &&component)
{
	DEBUG_ASSERT(entityToComponentIndex.find(entity) == entityToComponentIndex.end(), "The entity " + std::to_string(entity) + " already has a component of type " + typeid(T).name());

	components.push_back(component);
	entityToComponentIndex.emplace(entity, components.size() - 1);
	componentIndexToEntity.emplace(components.size() - 1, entity);
}
