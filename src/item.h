#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <unordered_map>

#include "ecs/ecs.h"
#include "graphics/texture_atlas.h"
#include "item_attribute.h"
#include "item_type.h"
#include "item_wrapper.h"
#include "position.h"

class Tile;

enum class ItemDataType
{
    Normal,
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
        Data() : _item(nullptr) {}

        Data(Item *item) : _item(item) {}

        virtual ItemDataType type() const noexcept = 0;
        virtual std::unique_ptr<Item::Data> copy() const = 0;

        Item *item() const noexcept
        {
            return _item;
        }

        virtual void setItem(Item *item)
        {
            _item = item;
        }

      protected:
        // The item that the data belongs to
        Item *_item;
    };

    Item(ItemTypeId serverId);
    ~Item();

    Item(const Item &other) = delete;
    Item &operator=(const Item &other) = delete;

    Item(Item &&other) noexcept;
    Item &operator=(Item &&other) noexcept;

    Item deepCopy() const;

    bool isContainer() const noexcept;

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
    const TextureInfo getTextureInfo(
        const Position &pos,
        TextureInfo::CoordinateType coordinateType = TextureInfo::CoordinateType::Normalized) const;
    inline ItemDataType itemDataType() const;

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

    ItemData::Container *getOrCreateContainer();

    template <class T>
    void setItemData(T &&itemData);
    void setItemData(ItemData::Container &&container);
    inline Item::Data *data() const;

    template <typename T, typename std::enable_if<std::is_base_of<Item::Data, T>::value>::type * = nullptr>
    inline T *getDataAs() const;

    const std::unordered_map<ItemAttribute_t, ItemAttribute> *attributes() const noexcept;

    bool operator==(const Item &rhs) const;

    // Wrapper converters for convenient _itemData access
    template <typename T, typename std::enable_if<std::is_base_of<ItemWrapper, T>::value>::type * = nullptr>
    T as();

    ItemType *itemType;
    bool selected = false;

  protected:
    friend class Tile;

    void registerEntity();

  private:
    ItemAttribute &getOrCreateAttribute(const ItemAttribute_t attributeType);

    const uint32_t getPatternIndex(const Position &pos) const;

    std::unique_ptr<std::unordered_map<ItemAttribute_t, ItemAttribute>> _attributes;
    std::unique_ptr<Item::Data> _itemData;
    // Subtype is either fluid type, count, subtype, or charges.
    uint8_t _subtype = 1;
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
    return _attributes && _attributes->size() > 0;
}

inline const int Item::getTopOrder() const noexcept
{
    return itemType->stackOrderIndex;
}

inline const std::unordered_map<ItemAttribute_t, ItemAttribute> *Item::attributes() const noexcept
{
    return _attributes.get();
}

inline ItemDataType Item::itemDataType() const
{
    return _itemData ? _itemData->type() : ItemDataType::Normal;
}

inline bool Item::operator==(const Item &rhs) const
{
    return itemType == rhs.itemType && _subtype == rhs._subtype &&
           util::unique_ptr_value_equals(_attributes, rhs._attributes);
}

template <class T>
inline void Item::setItemData(T &&itemData)
{
    static_assert(std::is_base_of<Item::Data, T>::value, "Bad type.");

    _itemData = std::make_unique<T>(std::move(itemData));
}

inline Item::Data *Item::data() const
{
    return _itemData.get();
}

template <typename T, typename std::enable_if<std::is_base_of<Item::Data, T>::value>::type *>
inline T *Item::getDataAs() const
{
    return static_cast<T *>(_itemData.get());
}

template <typename T, typename std::enable_if<std::is_base_of<ItemWrapper, T>::value>::type *>
T Item::as()
{
    return T(*this);
}

inline bool Item::isContainer() const noexcept
{
    return itemType->isContainer();
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
      public:
        struct HistoryParent
        {
        };

        struct TileParent
        {
            Position position;
            MapView *mapView;
        };

        using Parent = std::variant<std::monostate, HistoryParent, TileParent, Container *>;

      public:
        Container(uint16_t capacity) : _capacity(capacity) {}
        Container(uint16_t capacity, Item *item) : Item::Data(item), _capacity(capacity) {}
        Container(uint16_t capacity, Item *item, Parent parent) : Item::Data(item), _capacity(capacity) {}
        Container(uint16_t capacity, const std::vector<Item> &items);
        ~Container();

        Container(Container &&other) noexcept;
        Container &operator=(Container &&other) noexcept;

        ItemDataType type() const noexcept override;
        std::unique_ptr<Item::Data> copy() const override;
        void setItem(Item *item) override
        {
            _item = item;
            _parent = std::monostate{};
        }

        const std::vector<Item>::const_iterator findItem(std::function<bool(const Item &)> predicate) const;

        bool insertItemTracked(Item &&item, size_t index);
        Item dropItemTracked(size_t index);

        bool insertItem(Item &&item, size_t index);
        bool addItem(Item &&item);
        bool addItem(int index, Item &&item);
        bool removeItem(Item *item);
        bool removeItem(size_t index);

        Item dropItem(size_t index);

        Item &itemAt(size_t index);

        std::optional<size_t> indexOf(Item *item) const;

        const Item &itemAt(size_t index) const;
        const std::vector<Item> &items() const noexcept;
        size_t size() const noexcept;
        bool isFull() const noexcept;
        bool empty() const noexcept;

        uint16_t capacity() const noexcept;
        uint16_t volume() const noexcept;

        std::vector<Item> _items;

        uint16_t _capacity;

        std::vector<Parent> parents()
        {
            std::vector<Parent> result{_parent};
            while (std::holds_alternative<Container *>(result.back()))
            {
                result.emplace_back(std::get<Container *>(result.back())->_parent);
            }

            return result;
        }

        void setParent(MapView *mapView, Position position)
        {
            _parent = TileParent{position, mapView};
        }

        void setParent(Container *container)
        {
            _parent = container;
        }

      private:
        Parent _parent;
    };
} // namespace ItemData
