#pragma once

#include <QAbstractItemDelegate>
#include <QAbstractListModel>
#include <QPainter>
#include <QPixmap>
#include <QVariant>

#include <vector>

#include "../item_type.h"

struct ItemTypeModelItem
{
  ItemType *itemType;
  QPixmap pixmap;

  static ItemTypeModelItem fromServerId(uint32_t serverId);
};

Q_DECLARE_METATYPE(ItemTypeModelItem);

class QtItemTypeModel : public QAbstractListModel
{
  Q_OBJECT
private:
  using Item = ItemTypeModelItem;

public:
  QtItemTypeModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  void populate(std::vector<Item> &&items);

  std::vector<Item> _data;
};

class Delegate : public QAbstractItemDelegate
{
  Q_OBJECT
public:
  explicit Delegate(QObject *parent = nullptr);

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};