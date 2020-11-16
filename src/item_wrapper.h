#pragma once
//
// Utility wrappers for items that have extra data
//

class Item;
namespace ItemData
{
  struct Container;
}

struct ItemWrapper
{
  ItemWrapper(Item &item);
  Item &item;
};

struct ContainerItem : public ItemWrapper
{
  ContainerItem(Item &item);

  size_t containerSize() const;

  ItemData::Container *container() const;
};