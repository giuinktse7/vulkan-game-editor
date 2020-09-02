#include "item.h"

#include "items.h"
#include "ecs/ecs.h"
#include "ecs/item_animation.h"

#include "graphics/engine.h"
Item::Item(ItemTypeId itemTypeId)
		: subtype(1), selected(false)
{
	this->itemType = Items::items.getItemType(itemTypeId);
}

// std::optional<Entity> entity;
// ItemType *itemType;
// std::unordered_map<ItemAttribute_t, ItemAttribute> attributes;
// uint16_t subtype = 1;

Item::Item(Item &&other) noexcept
		: ecs::OptionalEntity(std::move(other)),
			itemType(other.itemType),
			attributes(std::move(other.attributes)),
			subtype(other.subtype),
			selected(other.selected)
{
	other.entityId.reset();
}

Item &Item::operator=(Item &&other) noexcept
{
	entityId = std::move(other.entityId);
	itemType = other.itemType;
	attributes = std::move(other.attributes);
	subtype = other.subtype;
	selected = other.selected;

	other.entityId.reset();

	return *this;
}

Item::~Item()
{
	auto entityId = getEntityId();
	if (entityId.has_value())
	{
		g_ecs.destroy(entityId.value());
		// Logger::debug() << "Destroying item " << std::to_string(this->getId()) << "(" << this->getName() << "), entity id: " << entityId.value() << std::endl;
	}
}

Item Item::deepCopy() const
{
	Item item(this->itemType->id);
	item.attributes = this->attributes;
	item.subtype = this->subtype;
	item.selected = this->selected;

	return item;
}

const TextureInfo Item::getTextureInfo(const Position &pos) const
{
	// TODO Add more pattern checks like hanging or cumulative item types
	const SpriteInfo &spriteInfo = itemType->appearance->getSpriteInfo();
	if (spriteInfo.hasAnimation() && isEntity())
	{
		auto c = g_ecs.getComponent<ItemAnimationComponent>(getEntityId().value());

		uint32_t width = spriteInfo.patternWidth;
		uint32_t height = spriteInfo.patternHeight;
		uint32_t depth = spriteInfo.patternDepth;

		uint32_t patternIndex = itemType->getPatternIndex(pos);
		uint32_t spriteIndex = patternIndex + c->state.phaseIndex * width * height * depth;

		uint32_t spriteId = spriteInfo.spriteIds.at(spriteIndex);

		return itemType->getTextureInfo(spriteId);
	}
	return itemType->getTextureInfo(pos);
}

const bool Item::isGround() const
{
	return itemType->isGroundTile();
}

uint16_t Item::getSubtype() const
{
	if (itemType->hasSubType())
	{
		return subtype;
	}

	return 0;
}
