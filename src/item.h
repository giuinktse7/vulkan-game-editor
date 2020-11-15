#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <unordered_map>

#include "ecs/ecs.h"
#include "graphics/texture.h"
#include "graphics/texture_atlas.h"
#include "item_attribute.h"
#include "items.h"
#include "position.h"

class Tile;

enum class ItemDataType
{
	Teleport,
	HouseDoor,
	Depot,
	Container
};

class Item : public ecs::OptionalEntity
{
	using ItemTypeId = uint32_t;

public:
	struct Data
	{
		virtual ItemDataType type() const noexcept = 0;
		virtual std::unique_ptr<Data> copy() const = 0;
	};

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

	uint32_t serverId() const noexcept;
	uint32_t clientId() const noexcept;
	const std::string name() const noexcept;
	const uint32_t weight() const noexcept;
	bool isGround() const noexcept;
	uint8_t subtype() const noexcept;
	inline uint8_t count() const noexcept;
	inline bool hasAttributes() const noexcept;
	inline const int getTopOrder() const noexcept;
	const TextureInfo getTextureInfo(const Position &pos) const;

	void setActionId(uint16_t id);
	void setUniqueId(uint16_t id);
	void setText(const std::string &text);
	void setText(std::string &&text);
	void setDescription(const std::string &description);
	void setDescription(std::string &&description);
	void setAttribute(ItemAttribute &&attribute);
	inline void setSubtype(uint8_t subtype) noexcept;
	inline void setCount(uint8_t count) noexcept;
	inline void setCharges(uint8_t charges) noexcept;

	template <typename T, typename std::enable_if<std::is_base_of<Data, T>::value>::type * = nullptr>
	void setItemData(T &&itemData);

	const std::unordered_map<ItemAttribute_t, ItemAttribute> &attributes() const noexcept;

	bool operator==(const Item &rhs) const;

protected:
	friend class Tile;

	void registerEntity();

private:
	std::unordered_map<ItemAttribute_t, ItemAttribute> _attributes;
	std::unique_ptr<Item::Data> _itemData;
	// Subtype is either fluid type, count, subtype, or charges.
	uint8_t _subtype = 1;

	ItemAttribute &getOrCreateAttribute(const ItemAttribute_t attributeType);

	const uint32_t getPatternIndex(const Position &pos) const;
};

inline uint32_t Item::serverId() const noexcept
{
	return itemType->id;
}

inline uint32_t Item::clientId() const noexcept
{
	return itemType->clientId;
}

inline const std::string Item::name() const noexcept
{
	return itemType->name;
}

inline const uint32_t Item::weight() const noexcept
{
	return itemType->weight;
}

inline bool operator!=(const Item &lhs, const Item &rhs)
{
	return !(lhs == rhs);
}

inline uint8_t Item::count() const noexcept
{
	return _subtype;
}

inline uint8_t Item::subtype() const noexcept
{
	return _subtype;
}

inline void Item::setSubtype(uint8_t subtype) noexcept
{
	this->_subtype = subtype;
}

inline void Item::setCount(uint8_t count) noexcept
{
	_subtype = count;
}

inline void Item::setCharges(uint8_t charges) noexcept
{
	_subtype = charges;
}

inline bool Item::hasAttributes() const noexcept
{
	return _attributes.size() > 0;
}

inline const int Item::getTopOrder() const noexcept
{
	return itemType->alwaysOnTopOrder;
}

inline const std::unordered_map<ItemAttribute_t, ItemAttribute> &Item::attributes() const noexcept
{
	return _attributes;
}

inline bool Item::operator==(const Item &rhs) const
{
	return itemType == rhs.itemType && _attributes == rhs._attributes && _subtype == rhs._subtype;
}

template <typename T, typename std::enable_if<std::is_base_of<Item::Data, T>::value>::type *>
inline void Item::setItemData(T &&itemData)
{
	// TODO Maybe weird
	_itemData = std::make_unique<T>(itemData);
}

namespace ItemData
{
	struct Teleport : public Item::Data
	{
		Teleport(const Position destination) : destination(destination) {}
		ItemDataType type() const noexcept override
		{
			return ItemDataType::Teleport;
		}

		std::unique_ptr<Item::Data> copy() const override
		{
			return std::make_unique<Teleport>(destination);
		}

		Position destination;
	};

	struct HouseDoor : public Item::Data
	{
		HouseDoor(uint8_t doorId) : doorId(doorId) {}
		ItemDataType type() const noexcept override
		{
			return ItemDataType::HouseDoor;
		}

		std::unique_ptr<Item::Data> copy() const override
		{
			return std::make_unique<HouseDoor>(doorId);
		}

		uint8_t doorId;
	};

	struct Depot : public Item::Data
	{
		Depot(uint16_t depotId) : depotId(depotId) {}
		ItemDataType type() const noexcept override
		{
			return ItemDataType::Depot;
		}

		std::unique_ptr<Item::Data> copy() const override
		{
			return std::make_unique<Depot>(depotId);
		}

		uint16_t depotId;
	};

	struct Container : public Item::Data
	{
		Container() {}
		Container(const std::vector<Item> &items)
		{
			for (const auto &item : items)
			{
				_items.emplace_back(item.deepCopy());
			}
		}

		ItemDataType type() const noexcept override
		{
			return ItemDataType::Container;
		}

		std::unique_ptr<Item::Data> copy() const override
		{
			return std::make_unique<Container>(_items);
		}

		void addItem(Item &&item)
		{
			_items.emplace_back(std::move(item));
		}

		Item &itemAt(size_t index)
		{
			return _items.at(index);
		}

		const Item &itemAt(size_t index) const
		{
			return _items.at(index);
		}

		const std::vector<Item> &items() const noexcept
		{
			return _items;
		}

		std::vector<Item> _items;
	};
} // namespace ItemData
