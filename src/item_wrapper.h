#pragma once
//
// Utility wrappers for items that have extra data
//

#include <optional>

class Item;
namespace ItemData
{
  struct Container;
}

struct ItemWrapper
{
  ItemWrapper(Item &item);
  Item *item;
};

struct ContainerItem : public ItemWrapper
{
  ContainerItem(const ContainerItem &other) noexcept;
  // ContainerItem &operator=(ContainerItem &&other) noexcept;

  static std::optional<ContainerItem> wrap(Item &item);

  size_t containerSize() const;
  size_t containerCapacity() const;
  bool full() const;
  bool empty() const;

  bool insertItem(Item &&item, size_t index);
  Item dropItem(size_t index);
  bool removeItem(size_t index);
  bool addItem(Item &&item);
  const Item &itemAt(size_t index) const;
  Item &itemAt(size_t index);

  ItemData::Container *container() const;

private:
  ContainerItem(Item &item);
};