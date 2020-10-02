#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <optional>

#include "graphics/texture.h"
#include "item_attribute.h"

#include "items.h"
#include "graphics/texture_atlas.h"
#include "position.h"
#include "ecs/ecs.h"

class Tile;

class Item : public ecs::OptionalEntity
{
	using ItemTypeId = uint32_t;

public:
	ItemType *itemType;
	bool selected = false;

	Item(ItemTypeId serverId);
	~Item();

	Item(const Item &other) = delete;
	Item &operator=(const Item &other) = delete;

	Item(Item &&other) noexcept;
	Item &operator=(Item &&other) noexcept;

	Item deepCopy() const;

	// Item(Item &&item) = default;

	uint32_t getId() const noexcept
	{
		return itemType->id;
	}

	uint32_t getClientId() const noexcept
	{
		return itemType->clientId;
	}

	const std::string getName() const noexcept
	{
		return itemType->name;
	}

	const uint32_t getWeight() const noexcept
	{
		return itemType->weight;
	}

	const TextureInfo getTextureInfo(const Position &pos) const;

	bool isGround() const noexcept;

	uint16_t getSubtype() const noexcept;

	inline bool hasAttributes() const noexcept
	{
		return attributes.size() > 0;
	}

	inline const int getTopOrder() const noexcept
	{
		return itemType->alwaysOnTopOrder;
	}

	const std::unordered_map<ItemAttribute_t, ItemAttribute> &getAttributes() const noexcept
	{
		return attributes;
	}

protected:
	friend class Tile;

	void registerEntity();

private:
	std::unordered_map<ItemAttribute_t, ItemAttribute> attributes;
	// Subtype is either fluid type, count, subtype, or charges.
	uint16_t subtype = 1;
};