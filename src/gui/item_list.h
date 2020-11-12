#pragma once

#include <QAbstractItemDelegate>
#include <QAbstractListModel>
#include <QListView>
#include <QPainter>
#include <QPixmap>
#include <QVariant>
#include <QWidget>

#include <vector>

#include "../item_type.h"

class QtItemTypeModel;

struct ItemTypeModelItem
{
  ItemType *itemType;
  QPixmap pixmap;

  static ItemTypeModelItem fromServerId(uint32_t serverId);
};

Q_DECLARE_METATYPE(ItemTypeModelItem);

class ItemList : public QListView
{
public:
  ItemList(QWidget *parent = nullptr);

  void addItem(uint32_t serverId);
  void addItems(std::vector<uint32_t> &&serverIds);
  void addItems(uint32_t from, uint32_t to);

  ItemTypeModelItem itemAtIndex(QModelIndex index);

private:
  QtItemTypeModel *_model;
};

class QtItemTypeModel : public QAbstractListModel
{
  Q_OBJECT
private:
public:
  QtItemTypeModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  void addItem(uint32_t serverId);
  void addItems(std::vector<uint32_t> &&serverIds);
  void addItems(uint32_t from, uint32_t to);

  void populate(std::vector<ItemTypeModelItem> &&items);

  std::vector<ItemTypeModelItem> _data;
};

class Delegate : public QAbstractItemDelegate
{
  Q_OBJECT
public:
  explicit Delegate(QObject *parent = nullptr);

  void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
  QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};