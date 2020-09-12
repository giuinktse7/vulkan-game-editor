#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <queue>
#include <memory>
#include <type_traits>

#include "../debug.h"
#include "../util.h"
#include "entity.h"
#include "component_array.h"

class ECS;

extern ECS g_ecs;

constexpr size_t MaxEntityComponents = 32;

namespace ecs
{
	using ComponentBitset = std::bitset<MaxEntityComponents>;

	class Entity
	{
	public:
		virtual ~Entity()
		{
			// Empty
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
		virtual void destroyEntity() = 0;
		virtual bool isEntity() const = 0;

		virtual void setEntityId(EntityId id) = 0;
	};

	class OptionalEntity : public Entity
	{
	public:
		std::optional<ecs::EntityId> getEntityId() const override;
		bool isEntity() const override;
		void destroyEntity() override;

		void setEntityId(ecs::EntityId id) override;

	protected:
		std::optional<EntityId> entityId;
	};

	class System
	{
	public:
		virtual void update() = 0;
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

		virtual std::vector<const char *> getRequiredComponents() = 0;

		ecs::ComponentBitset componentBitset;
		std::unordered_set<EntityId> entities;
	};
} // namespace ecs

class ECS
{
	using ComponentType = uint8_t;

public:
	ecs::EntityId createEntity();

	template <typename T>
	void registerComponent()
	{
		const char *typeName = typeid(T).name();

		DEBUG_ASSERT(componentTypes.find(typeName) == componentTypes.end(), "The component type " + std::string(typeName) + " has already been registered.");

		componentTypes.insert({typeName, nextComponentType});
		componentArrays[typeName] = std::make_unique<ComponentArray<T>>();

		++nextComponentType;
	}

	template <typename T>
	ecs::System *registerSystem()
	{
		const char *typeName = typeid(T).name();
		DEBUG_ASSERT(systems.find(typeName) == systems.end(), "The system '" + std::string(typeName) + "' is already registered.");

		std::unique_ptr<ecs::System> system;

		// Pointer magic to cast the T (which should be deriving from ecs::System) to a ecs::System
		std::unique_ptr<T> basePointer = std::make_unique<T>();
		ecs::System *tmp = dynamic_cast<ecs::System *>(basePointer.get());
		if (tmp != nullptr)
		{
			basePointer.release();
			system.reset(tmp);
		}

		for (const auto &componentTypeName : system->getRequiredComponents())
		{
			DEBUG_ASSERT(componentTypes.find(componentTypeName) != componentTypes.end(), "The component type '" + std::string(componentTypeName) + "' is not registered.");

			ECS::ComponentType bit = componentTypes[componentTypeName];
			system->componentBitset.set(bit);
		}

		systems[typeName] = std::move(system);
		return systems[typeName].get();
	}

	template <typename T>
	ComponentType getComponentType()
	{
		const char *typeName = typeid(T).name();

		DEBUG_ASSERT(componentTypes.find(typeName) != componentTypes.end(), "The component type '" + std::string(typeName) + "' is not registered.");

		return componentTypes[typeName];
	}

	template <typename T>
	T *getComponent(ecs::EntityId entity)
	{
		ComponentArray<T> *componentArray = getComponentArray<T>();
		if (componentArray->hasComponent(entity))
		{
			T *component = componentArray->getComponent(entity);
			return component;
		}
		else
		{
			return nullptr;
		}
	}

	template <typename T>
	T &getSystem()
	{
		const char *typeName = typeid(T).name();

		DEBUG_ASSERT(systems.find(typeName) != systems.end(), "There is no registered '" + std::string(typeName) + "' system.");

		ecs::System &system = *systems.at(typeName).get();
		return dynamic_cast<T &>(system);
	}

	template <typename T>
	T *addComponent(ecs::EntityId entity, T component)
	{
		ComponentArray<T> *componentArray = getComponentArray<T>();
		componentArray->addComponent(entity, component);

		setEntityComponentBit<T>(entity);

		return getComponent<T>(entity);
	}

	template <typename T>
	void removeComponent(ecs::EntityId entity)
	{
		ComponentArray<T> *array = getComponentArray<T>();
		array->removeComponent(entity);

		unsetEntityComponentBit<T>(entity);
	}

	template <typename T>
	bool systemRequiresComponent(ecs::System &system)
	{
		return system.componentBitset.test(componentTypes.at(typeid(T).name()));
	}

	template <typename T>
	void removeAllComponents()
	{
		ComponentArray<T> *array = getComponentArray<T>();
		array->clear();

		for (const auto &entry : systems)
		{
			ecs::System *system = entry.second.get();
			if (systemRequiresComponent<T>(*system))
			{
				system->clear();
			}
		}
	}

	void destroy(ecs::EntityId entity)
	{
		for (const auto &entry : componentArrays)
		{
			const auto &componentArray = entry.second;
			componentArray->entityDestroyed(entity);
		}

		for (const auto &entry : systems)
		{
			const auto &system = entry.second;
			system->entities.erase(entity);
		}

		entityIdQueue.emplace(entity);
		entityComponentBitsets.erase(entity);
	}

	/*
		Destroy entities that are marked for destruction.
	*/
	void destroyMarkedEntities()
	{
		for (const ecs::EntityId id : entityIdsMarkedForDestruction)
		{
			destroy(id);
		}
	}

	/*
		Marks an entity for destruction. This is useful in places where entities are
		iterated and potentially being destroyed in the iteration.
	*/
	void markEntityForDestruction(ecs::EntityId id)
	{
		entityIdsMarkedForDestruction.emplace(id);
	}

	bool isMarkedForDestruction(ecs::EntityId id) const
	{
		return entityIdsMarkedForDestruction.find(id) != entityIdsMarkedForDestruction.end();
	}

private:
	uint32_t nextComponentType = 0;
	uint32_t entityCounter = 0;

	/*
		Used to mark entity ids as destroyed when iterating over entities,
		because entities can't be destroyed while they are being iterated over.
	*/
	std::unordered_set<ecs::EntityId> entityIdsMarkedForDestruction;

	std::queue<ecs::EntityId> entityIdQueue;
	std::unordered_map<ecs::EntityId, ecs::ComponentBitset> entityComponentBitsets;

	std::unordered_map<const char *, ComponentType> componentTypes;
	std::unordered_map<const char *, std::unique_ptr<IComponentArray>> componentArrays;

	std::unordered_map<const char *, std::unique_ptr<ecs::System>> systems;

	template <typename T>
	ComponentArray<T> *getComponentArray()
	{
		const char *typeName = typeid(T).name();
		DEBUG_ASSERT(componentTypes.find(typeName) != componentTypes.end(), "The component type '" + std::string(typeName) + "' is not registered.");

		return dynamic_cast<ComponentArray<T> *>(componentArrays.at(typeName).get());
	}

	template <typename T>
	void setEntityComponentBit(ecs::EntityId entity)
	{
		ECS::ComponentType componentType = getComponentType<T>();
		auto &bitset = entityComponentBitsets.at(entity);
		bitset.set(componentType);

		onEntityBitsetChanged(entity, bitset);
	}

	template <typename T>
	void unsetEntityComponentBit(ecs::EntityId entity)
	{
		ECS::ComponentType componentType = getComponentType<T>();
		auto &bitset = entityComponentBitsets.at(entity);
		bitset.reset(componentType);

		onEntityBitsetChanged(entity, bitset);
	}

	void onEntityBitsetChanged(ecs::EntityId entity, ecs::ComponentBitset bitset)
	{
		for (const auto &entry : systems)
		{
			ecs::System *system = entry.second.get();
			if ((bitset & system->componentBitset) == system->componentBitset)
			{
				system->entities.emplace(entity);
			}
			else
			{
				system->entities.erase(entity);
			}
		}
	}
};

template <typename T>
bool ecs::Entity::hasComponent() const
{
	return isEntity() && g_ecs.getComponent<T>(getEntityId().value()) != nullptr;
}

template <typename T>
T *ecs::Entity::getComponent() const
{
	return isEntity() ? g_ecs.getComponent<T>(getEntityId().value()) : nullptr;
}

template <typename T>
T *ecs::Entity::addComponent(T component) const
{
	DEBUG_ASSERT(isEntity(), "A non-existing entity can not add a component in ECS.");

	return g_ecs.addComponent(getEntityId().value(), component);
}