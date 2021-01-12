#pragma once

#include <optional>
#include <unordered_map>

#include "graphics/texture_atlas.h"
#include "item_animation.h"
#include "item_attribute.h"
#include "item_data.h"
#include "item_type.h"
#include "item_wrapper.h"
#include "position.h"

class Tile;
struct Container;

class Item
{
    using ItemTypeId = uint32_t;

  public:
    Item(ItemTypeId serverId);
    ~Item();

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

    Container *getOrCreateContainer();

    template <class T>
    void setItemData(T &&itemData);
    void setItemData(Container &&container);
    inline ItemData *data() const;

    template <typename T>
    inline T *getDataAs() const;

    const std::unordered_map<ItemAttribute_t, ItemAttribute> *attributes() const noexcept;

    bool operator==(const Item &rhs) const;

    ItemAnimation *animation() const noexcept;
    void animate() const;

    uint32_t guid() const noexcept;

    // Wrapper converters for convenient _itemData access
    template <typename T, typename std::enable_if<std::is_base_of<ItemWrapper, T>::value>::type * = nullptr>
    T as();

    ItemType *itemType;
    bool selected = false;

  protected:
    friend class Tile;

  private:
    Item(const Item &other);

    ItemAttribute &getOrCreateAttribute(const ItemAttribute_t attributeType);

    const uint32_t getPatternIndex(const Position &pos) const;

    mutable std::shared_ptr<ItemAnimation> _animation = nullptr;

    std::unique_ptr<std::unordered_map<ItemAttribute_t, ItemAttribute>> _attributes;
    std::unique_ptr<ItemData> _itemData;
    // Subtype is either fluid type, count, subtype, or charges.
    uint8_t _subtype = 1;

    uint32_t _guid;
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
    static_assert(std::is_base_of<ItemData, T>::value, "Bad type.");

    _itemData = std::make_unique<T>(std::move(itemData));
}

template <typename T>
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
