#include "item_data.h"

#include "item.h"
#include "items.h"

Container::Container(uint16_t capacity, const std::vector<Item> &items)
    : _capacity(capacity)
{
    DEBUG_ASSERT(capacity >= items.size(), "Too many items.");

    for (const auto &item : items)
    {
        _items.emplace_back(item.deepCopy());
    }
}

Container::Container(Container &&other) noexcept
    : _items(std::move(other._items)), _capacity(std::move(other._capacity))
{
}

Container::~Container() {}

Container &Container::operator=(Container &&other) noexcept
{
    _items = std::move(other._items);
    _capacity = std::move(other._capacity);
    return *this;
}

ItemDataType Container::type() const noexcept
{
    return ItemDataType::Container;
}

std::unique_ptr<ItemData> Container::copy() const
{
    return std::make_unique<Container>(_capacity, _items);
}

bool Container::isFull() const noexcept
{
    return _capacity == _items.size();
}

bool Container::empty() const noexcept
{
    return _items.empty();
}

bool Container::insertItemTracked(Item &&item, size_t index)
{
    if (isFull())
        return false;

    auto itemLocation = _items.emplace(_items.begin() + index, std::move(item));
    while (itemLocation != _items.end())
    {
        Items::items.itemMoved(&(*itemLocation));
        ++itemLocation;
    }

    return true;
}

bool Container::insertItem(Item &&item, size_t index)
{
    if (isFull())
        return false;

    auto itemLocation = _items.emplace(_items.begin() + index, std::move(item));
    return true;
}

bool Container::addItem(Item &&item)
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
            item.setItemData(Container(item.itemType->volume, &item, this));
        }
        else
        {
            auto container = item.getDataAs<Container>();
            container->setItem(&item);
            container->setParent(this);
        }
    }

    return true;
}

bool Container::addItem(int index, Item &&item)
{
    if (isFull())
        return false;

    _items.emplace(_items.begin() + index, std::move(item));
    return true;
}

bool Container::removeItem(size_t index)
{
    if (index > size())
        return false;

    _items.erase(_items.begin() + index);
    return true;
}

bool Container::removeItem(Item *item)
{
    auto found = std::find_if(_items.begin(), _items.end(), [item](const Item &_item) { return item == &_item; });

    // check that there actually is a 3 in our vector
    if (found == _items.end())
    {
        return false;
    }

    _items.erase(found);
    return true;
}

Item Container::dropItemTracked(size_t index)
{
    Item item(std::move(_items.at(index)));

    auto itemLocation = _items.erase(_items.begin() + index);
    while (itemLocation != _items.end())
    {
        Items::items.itemMoved(&(*itemLocation));
        ++itemLocation;
    }

    return item;
}

Item Container::dropItem(size_t index)
{
    Item item(std::move(_items.at(index)));
    _items.erase(_items.begin() + index);

    return item;
}

Item &Container::itemAt(size_t index)
{
    return _items.at(index);
}

const Item &Container::itemAt(size_t index) const
{
    return _items.at(index);
}

std::optional<size_t> Container::indexOf(Item *item) const
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

const std::vector<Item>::const_iterator Container::findItem(std::function<bool(const Item &)> predicate) const
{
    return std::find_if(_items.begin(), _items.end(), predicate);
}

const std::vector<Item> &Container::items() const noexcept
{
    return _items;
}

size_t Container::size() const noexcept
{
    return _items.size();
}

uint16_t Container::capacity() const noexcept
{
    return _capacity;
}

uint16_t Container::volume() const noexcept
{
    return _capacity;
}