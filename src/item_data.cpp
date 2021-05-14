#include "item_data.h"

#include "item.h"
#include "items.h"

Container::Container(uint16_t capacity, const std::vector<std::unique_ptr<Item>> &items)
    : _capacity(capacity)
{
    DEBUG_ASSERT(capacity >= items.size(), "Too many items.");

    for (const auto &item : items)
    {
        _items.emplace_back(std::make_unique<Item>(item->deepCopy()));
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

    auto itemLocation = _items.emplace(_items.begin() + index, std::make_unique<Item>(std::move(item)));

    Items::items.containerChanged(this->item(), ContainerChange::inserted(static_cast<uint8_t>(index)));

    while (itemLocation != _items.end())
    {
        Items::items.itemMoved(&(**itemLocation));
        ++itemLocation;
    }

    return true;
}

bool Container::insertItem(Item &&item, size_t index)
{
    if (isFull())
        return false;

    auto itemLocation = _items.emplace(_items.begin() + index, std::make_unique<Item>(std::move(item)));
    return true;
}

bool Container::addItem(Item &&item)
{
    if (isFull())
        return false;

    VME_LOG_D("Add item " << item.name() << " to container " << this->item()->name());

    bool isContainer = item.isContainer();

    _items.emplace_back(std::make_unique<Item>(std::move(item)));
    if (isContainer)
    {
        auto &item = _items.back();
        if (item->itemDataType() != ItemDataType::Container)
        {
            item->setItemData(Container(item->itemType->volume, &(*item), this));
        }
        else
        {
            auto container = item->getDataAs<Container>();
            container->setItem(&(*item));
            container->setParent(this);
        }
    }

    return true;
}

bool Container::addItem(int index, Item &&item)
{
    if (isFull())
        return false;

    _items.emplace(_items.begin() + index, std::make_unique<Item>(std::move(item)));
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
    auto found = findItem([item](const Item &_item) { return item == &_item; });

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
    DEBUG_ASSERT(index <= UINT8_MAX, "index too large.");

    Item item(std::move(*_items.at(index)));

    auto itemLocation = _items.erase(_items.begin() + index);

    Items::items.containerChanged(this->item(), ContainerChange::removed(static_cast<uint8_t>(index)));

    while (itemLocation != _items.end())
    {
        Items::items.itemMoved(&(**itemLocation));
        ++itemLocation;
    }

    return item;
}

void Container::moveItemTracked(size_t fromIndex, size_t toIndex)
{
    DEBUG_ASSERT(fromIndex <= UINT8_MAX, "fromIndex too large.");
    DEBUG_ASSERT(toIndex <= UINT8_MAX, "toIndex too large.");

    util::moveByRotate(_items, fromIndex, toIndex);

    Items::items.containerChanged(this->item(), ContainerChange::moveInSameContainer(static_cast<uint8_t>(fromIndex), static_cast<uint8_t>(toIndex)));

    auto from = std::min(fromIndex, toIndex);
    auto to = std::max(fromIndex, toIndex);

    auto fromIt = _items.begin() + from;
    auto toIt = _items.begin() + to;

    while (fromIt != toIt)
    {
        Items::items.itemMoved(&(**fromIt));
        ++fromIt;
    }
}

bool Container::hasNonFullContainerAtIndex(size_t index)
{
    if (index >= size())
    {
        return false;
    }

    auto &slotItem = itemAt(index);
    if (!slotItem.isContainer())
    {
        return false;
    }

    auto slotContainer = slotItem.getOrCreateContainer();
    if (slotContainer->isFull())
    {
        return false;
    }

    return true;
}

Item Container::dropItem(size_t index)
{
    Item item(std::move(*_items.at(index)));
    _items.erase(_items.begin() + index);

    return item;
}

void Container::moveItem(size_t fromIndex, size_t toIndex)
{
    DEBUG_ASSERT(fromIndex <= UINT8_MAX, "fromIndex too large.");
    DEBUG_ASSERT(toIndex <= UINT8_MAX, "toIndex too large.");

    util::moveByRotate(_items, fromIndex, toIndex);
}

Item &Container::itemAt(size_t index)
{
    return *_items.at(index);
}

const Item &Container::itemAt(size_t index) const
{
    return *_items.at(index);
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

const std::vector<std::unique_ptr<Item>>::const_iterator Container::findItem(std::function<bool(const Item &)> predicate) const
{
    return std::find_if(_items.begin(), _items.end(), [predicate](const std::unique_ptr<Item> &_item) { return predicate(*_item); });
}

const std::vector<std::unique_ptr<Item>> &Container::items() const noexcept
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