#include "item_wrapper.h"

#include "item.h"

ItemWrapper::ItemWrapper(Item &item) : item(item) {}

ContainerItem::ContainerItem(Item &item) : ItemWrapper(item)
{
  assert(item.isContainer());
  if (item.itemDataType() != ItemDataType::Container)
  {
    ItemData::Container container;
    item.setItemData(std::move(container));
  }
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
