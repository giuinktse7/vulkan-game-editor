#pragma once

#include <stdint.h>
#include "graphics/texture.h"
#include "graphics/appearances.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <optional>

#include "item_attribute.h"

#include "items.h"
#include "graphics/texture_atlas.h"
#include "position.h"
#include "ecs/ecs.h"

class Appearances;

class Item : public ecs::OptionalEntity
{
	using ItemTypeId = uint32_t;

public:
	ItemType *itemType;
	bool selected;

	Item(ItemTypeId serverId);
	~Item();

	Item(const Item &other) = delete;
	Item &operator=(const Item &other) = delete;

	Item(Item &&other) noexcept;
	Item &operator=(Item &&other) noexcept;

	Item deepCopy() const;

	// Item(Item &&item) = default;

	uint32_t getId() const
	{
		return itemType->id;
	}

	uint32_t getClientId() const
	{
		return itemType->clientId;
	}

	const std::string getName() const
	{
		return itemType->name;
	}

	const uint32_t getWeight() const
	{
		return itemType->weight;
	}

	const TextureInfo getTextureInfo(const Position &pos) const;

	const bool isGround() const;

	uint16_t getSubtype() const;

	bool hasAttributes() const
	{
		return attributes.size() > 0;
	}

	const inline int getTopOrder() const
	{
		return itemType->alwaysOnTopOrder;
	}

	const std::unordered_map<ItemAttribute_t, ItemAttribute> &getAttributes() const
	{
		return attributes;
	}

private:
	std::unordered_map<ItemAttribute_t, ItemAttribute> attributes;
	// Subtype is either fluid type, count, subtype, or charges.
	uint16_t subtype = 1;
};