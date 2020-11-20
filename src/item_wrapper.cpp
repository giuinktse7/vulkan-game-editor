#include "item_wrapper.h"

#include "item.h"

ItemWrapper::ItemWrapper(Item &item) : item(item) {}

ContainerItem::ContainerItem(Item &item) : ItemWrapper(item)
{
  assert(item.isContainer());
  if (item.itemDataType() != ItemDataType::Container)
  {
    item.setItemData(ItemData::Container(item.itemType->volume));
  }
}

ContainerItem::ContainerItem(const ContainerItem &other) noexcept
    : ItemWrapper(other.item) {}

std::optional<ContainerItem> ContainerItem::wrap(Item &item)
{
  std::optional<ContainerItem> container;
  if (item.isContainer())
  {
    container.emplace(ContainerItem(item));
  }

  return container;
}

ItemData::Container *ContainerItem::container() const
{
  return dynamic_cast<ItemData::Container *>(item.data());
}

size_t ContainerItem::containerSize() const
{
  return container()->size();
}

size_t ContainerItem::containerCapacity() const
{
  return item.itemType->volume;
}

bool ContainerItem::full() const
{
  return container()->size() == containerCapacity();
}

bool ContainerItem::empty() const
{
  return container()->size() == 0;
}

bool ContainerItem::addItem(Item &&item)
{
  return container()->addItem(std::move(item));
}

const Item &ContainerItem::itemAt(size_t index) const
{
  return container()->itemAt(index);
}