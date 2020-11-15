#include "item.h"

#include "ecs/ecs.h"
#include "ecs/item_animation.h"
#include "items.h"
#include "util.h"

namespace
{
	enum class StackSizeOffset
	{
		One = 0,
		Two = 1,
		Three = 2,
		Four = 3,
		Five = 4,
		Ten = 5,
		TwentyFive = 6,
		Fifty = 7
	};
}

Item::Item(ItemTypeId itemTypeId)
		: itemType(Items::items.getItemTypeByServerId(itemTypeId))
{
	registerEntity();
}

Item::Item(Item &&other) noexcept
		: ecs::OptionalEntity(std::move(other)),
			itemType(other.itemType),
			selected(other.selected),
			_attributes(std::move(other._attributes)),
			_subtype(other._subtype),
			_itemData(std::move(other._itemData))
{
	// Necessary because the move does not remove the entity id of 'other'
	other.entityId.reset();
}

Item &Item::operator=(Item &&other) noexcept
{
	entityId = std::move(other.entityId);
	itemType = other.itemType;
	_attributes = std::move(other._attributes);
	_subtype = other._subtype;
	_itemData = std::move(other._itemData);
	selected = other.selected;

	// Necessary because the move does not remove the entity id of 'other'
	other.entityId.reset();

	return *this;
}

Item::~Item()
{
	auto entityId = getEntityId();
	if (entityId.has_value())
	{
		g_ecs.destroy(entityId.value());
		// Logger::debug() << "Destroying item " << std::to_string(this->getId()) << "(" << this->name() << "), entity id: " << entityId.value() << std::endl;
	}
}

Item Item::deepCopy() const
{
	Item item(this->itemType->id);
	item._attributes = this->_attributes;
	item._subtype = this->_subtype;
	if (_itemData) {
		item._itemData = _itemData->copy();
	}
	item.selected = this->selected;

	return item;
}

const TextureInfo Item::getTextureInfo(const Position &pos) const
{
	// TODO Add more pattern checks like hanging item types

	uint32_t offset = getPatternIndex(pos);
	const SpriteInfo &spriteInfo = itemType->appearance->getSpriteInfo(0);
	if (spriteInfo.hasAnimation() && isEntity())
	{
		auto c = g_ecs.getComponent<ItemAnimationComponent>(getEntityId().value());
		offset += c->state.phaseIndex * spriteInfo.patternSize;
	}

	uint32_t spriteId = spriteInfo.spriteIds.at(offset);
	return itemType->getTextureInfo(spriteId);
}

const uint32_t Item::getPatternIndex(const Position &pos) const
{
	const SpriteInfo &spriteInfo = itemType->appearance->getSpriteInfo();
	if (!itemType->isStackable())
		return itemType->getPatternIndex(pos);

	// For stackable items

	// Amount of sprites for the different counts
	uint8_t stackSpriteCount = spriteInfo.patternSize;
	if (stackSpriteCount == 1)
		return 0;

	int itemCount = count();
	if (itemCount <= 5)
		return itemCount - 1;
	else if (itemCount < 10)
		return to_underlying(StackSizeOffset::Five);
	else if (itemCount < 25)
		return to_underlying(StackSizeOffset::Ten);
	else if (itemCount < 50)
		return to_underlying(StackSizeOffset::TwentyFive);
	else
		return to_underlying(StackSizeOffset::Fifty);
}

bool Item::isGround() const noexcept
{
	return itemType->isGroundTile();
}

void Item::setAttribute(ItemAttribute &&attribute)
{
	_attributes.emplace(attribute.type(), std::move(attribute));
}

void Item::setActionId(uint16_t id)
{
	getOrCreateAttribute(ItemAttribute_t::ActionId).setInt(id);
}

void Item::setUniqueId(uint16_t id)
{
	getOrCreateAttribute(ItemAttribute_t::UniqueId).setInt(id);
}

void Item::setText(const std::string &text)
{
	getOrCreateAttribute(ItemAttribute_t::Text).setString(text);
}

void Item::setText(std::string &&text)
{
	getOrCreateAttribute(ItemAttribute_t::Text).setString(std::move(text));
}

void Item::setDescription(const std::string &description)
{
	getOrCreateAttribute(ItemAttribute_t::Description).setString(description);
}

void Item::setDescription(std::string &&description)
{
	getOrCreateAttribute(ItemAttribute_t::Description).setString(std::move(description));
}

ItemAttribute &Item::getOrCreateAttribute(const ItemAttribute_t attributeType)
{
	_attributes.try_emplace(attributeType, attributeType);
	return _attributes.at(attributeType);
}

void Item::registerEntity()
{
	DEBUG_ASSERT(!entityId.has_value(), "Attempted to register an entity that already has an entity id.");

	if (itemType->hasAnimation())
	{
		ecs::EntityId entityId = assignNewEntityId();

		auto animation = itemType->appearance->getSpriteInfo().animation();
		g_ecs.addComponent(entityId, ItemAnimationComponent(animation));
	}
}
