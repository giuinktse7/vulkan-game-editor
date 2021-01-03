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

	if (_itemData)
	{
		_itemData->setItem(this);
	}
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

	if (_itemData)
	{
		_itemData->setItem(this);
	}

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

	item.entityId = entityId;

	if (_attributes)
	{
		auto attributeCopy = *_attributes.get();
		item._attributes = std::make_unique<std::unordered_map<ItemAttribute_t, ItemAttribute>>(std::move(attributeCopy));
	}

	item._subtype = this->_subtype;
	if (_itemData)
	{
		item._itemData = _itemData->copy();
		item._itemData->setItem(nullptr);
	}
	item.selected = this->selected;

	return item;
}

const TextureInfo Item::getTextureInfo(const Position &pos, TextureInfo::CoordinateType coordinateType) const
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
	return itemType->getTextureInfo(spriteId, coordinateType);
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
	if (!_attributes)
	{
		_attributes = std::make_unique<std::unordered_map<ItemAttribute_t, ItemAttribute>>();
	}

	_attributes->emplace(attribute.type(), std::move(attribute));
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
	if (!_attributes)
	{
		_attributes = std::make_unique<std::unordered_map<ItemAttribute_t, ItemAttribute>>();
	}

	_attributes->try_emplace(attributeType, attributeType);
	return _attributes->at(attributeType);
}

void Item::setItemData(ItemData::Container &&container)
{
	_itemData = std::make_unique<ItemData::Container>(std::move(container));
}

ItemData::Container *Item::getOrCreateContainer()
{
	DEBUG_ASSERT(isContainer(), "Must be container.");
	if (itemDataType() != ItemDataType::Container)
	{
		_itemData = std::make_unique<ItemData::Container>(itemType->volume, this);
	}

	return getDataAs<ItemData::Container>();
}

void Item::registerEntity()
{
	DEBUG_ASSERT(!entityId.has_value(), "Attempted to register an entity that already has an entity id.");
	ecs::EntityId entityId = assignNewEntityId();

	if (itemType->hasAnimation())
	{
		auto animation = itemType->appearance->getSpriteInfo().animation();
		g_ecs.addComponent(entityId, ItemAnimationComponent(animation));
	}
}

//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>ItemData>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>

ItemData::Container::Container(uint16_t capacity, const std::vector<Item> &items)
		: _capacity(capacity)
{
	DEBUG_ASSERT(capacity >= items.size(), "Too many items.");

	for (const auto &item : items)
	{
		_items.emplace_back(item.deepCopy());
	}
}

ItemData::Container::Container(Container &&other) noexcept
		: _items(std::move(other._items)), _capacity(std::move(other._capacity))
{
}

ItemData::Container::~Container()
{
}

ItemData::Container &ItemData::Container::operator=(Container &&other) noexcept
{
	_items = std::move(other._items);
	_capacity = std::move(other._capacity);
	return *this;
}

ItemDataType ItemData::Container::type() const noexcept
{
	return ItemDataType::Container;
}

std::unique_ptr<Item::Data> ItemData::Container::copy() const
{
	return std::make_unique<Container>(_capacity, _items);
}

bool ItemData::Container::isFull() const noexcept
{
	return _capacity == _items.size();
}

bool ItemData::Container::empty() const noexcept
{
	return _items.empty();
}

bool ItemData::Container::insertItem(Item &&item, size_t index)
{
	if (isFull())
		return false;

	_items.emplace(_items.begin() + index, std::move(item));
	return true;
}

bool ItemData::Container::addItem(Item &&item)
{
	if (isFull())
		return false;

	bool isContainer = item.isContainer();

	_items.emplace_back(std::move(item));
	if (isContainer)
	{
		auto &item = _items.back();
		if (item.itemDataType() != ItemDataType::Container)
		{
			item.setItemData(ItemData::Container(item.itemType->volume, &item, this));
		}
		else
		{
			auto container = item.getDataAs<ItemData::Container>();
			container->setItem(&item);
			container->setParent(this);
		}
	}

	return true;
}

bool ItemData::Container::addItem(int index, Item &&item)
{
	if (isFull())
		return false;

	_items.emplace(_items.begin() + index, std::move(item));
	return true;
}

bool ItemData::Container::removeItem(size_t index)
{
	if (index > size())
		return false;

	_items.erase(_items.begin() + index);
	return true;
}

bool ItemData::Container::removeItem(Item *item)
{
	auto found = std::find_if(
			_items.begin(),
			_items.end(),
			[item](const Item &_item) { return item == &_item; });

	// check that there actually is a 3 in our vector
	if (found == _items.end())
	{
		return false;
	}

	_items.erase(found);
	return true;
}

Item ItemData::Container::dropItem(size_t index)
{
	Item item(std::move(_items.at(index)));
	_items.erase(_items.begin() + index);

	return item;
}

Item &ItemData::Container::itemAt(size_t index)
{
	return _items.at(index);
}

const Item &ItemData::Container::itemAt(size_t index) const
{
	return _items.at(index);
}

std::optional<size_t> ItemData::Container::indexOf(Item *item) const
{
	auto found = findItem([item](const Item &_item) { return item == &_item; });
	if (found != _items.end())
	{
		return static_cast<size_t>(found - _items.begin());
	}
	else
	{
		return std::nullopt;
	}
}

const std::vector<Item>::const_iterator ItemData::Container::findItem(std::function<bool(const Item &)> predicate) const
{
	return std::find_if(_items.begin(), _items.end(), predicate);
}

const std::vector<Item> &ItemData::Container::items() const noexcept
{
	return _items;
}

size_t ItemData::Container::size() const noexcept
{
	return _items.size();
}

uint16_t ItemData::Container::capacity() const noexcept
{
	return _capacity;
}

uint16_t ItemData::Container::volume() const noexcept
{
	return _capacity;
}
