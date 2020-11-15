#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <stdint.h>
#include <unordered_map>

#include "graphics/texture.h"
#include "item_attribute.h"

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

	template <typename T, typename std::enable_if<std::is_base_of<Data, T>::value>::type * = nullptr>
	void setItemData(T &&itemData);
	std::unique_ptr<Item::Data> _itemData;

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

	uint8_t getSubtype() const noexcept;

	inline bool hasAttributes() const noexcept
	{
		return attributes.size() > 0;
	}

	inline const int getTopOrder() const noexcept
	{
		return itemType->alwaysOnTopOrder;
	}

	void setActionId(uint16_t id);
	void setUniqueId(uint16_t id);
	void setText(const std::string &text);
	void setText(std::string &&text);
	void setDescription(const std::string &description);
	void setDescription(std::string &&description);

	const vme_unordered_map<ItemAttribute_t, ItemAttribute> &getAttributes() const noexcept
	{
		return attributes;
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
