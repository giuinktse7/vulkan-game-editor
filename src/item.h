#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <stdint.h>
#include <unordered_map>

#include "graphics/texture.h"
#include "item_attribute.h"

#include "ecs/ecs.h"
#include "graphics/texture_atlas.h"
#include "items.h"
#include "position.h"

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

	uint16_t serverId() const noexcept
	{
		return itemType->id;
	}

	uint32_t clientId() const noexcept
	{
		return itemType->clientId;
	}

	const std::string name() const noexcept
	{
		return itemType->name;
	}

	const uint32_t weight() const noexcept
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

	inline uint16_t count() const noexcept;

	inline void setCount(uint16_t count) noexcept;

protected:
	friend class Tile;

	void registerEntity();

private:
	std::unordered_map<ItemAttribute_t, ItemAttribute> attributes;
	// Subtype is either fluid type, count, subtype, or charges.
	uint16_t subtype = 1;

	const uint32_t getPatternIndex(const Position &pos) const;
};

inline uint16_t Item::count() const noexcept
{
	return subtype;
}

inline void Item::setCount(uint16_t count) noexcept
{
	subtype = count;
}