#include "item_list.h"

#include "../logger.h"
#include "../items.h"

#include "qt_util.h"

ItemTypeModelItem ItemTypeModelItem::fromServerId(uint16_t serverId)
{
  ItemTypeModelItem value;
  value.itemType = Items::items.getItemType(serverId);
  value.pixmap = QtUtil::itemPixmap(serverId);

  return value;
}

void QtItemTypeModel::populate(std::vector<QtItemTypeModel::Item> &&items)
{
  beginResetModel();
  _data = std::move(items);
  endResetModel();
}

int QtItemTypeModel::rowCount(const QModelIndex &parent) const
{
  return _data.size();
}

QVariant QtItemTypeModel::data(const QModelIndex &index, int role) const
{
  // VME_LOG_D("data(" << index.row() << ", " << role << ")");
  if (index.row() < 0 || index.row() >= _data.size())
    return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole)
  {
    QVariant result;
    result.setValue(_data.at(index.row()));
    return result;
  }

  return QVariant();
}

QVariant QtItemTypeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  // VME_LOG_D("headerData " << section << ", " << orientation);
  return QVariant();
}

Qt::ItemFlags QtItemTypeModel::flags(const QModelIndex &index) const
{
  return Qt::ItemFlag::ItemIsEnabled;
}

Delegate::Delegate(QObject *parent) : QAbstractItemDelegate(parent)
{
}

void Delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  ItemTypeModelItem data = qvariant_cast<ItemTypeModelItem>(index.data());

  painter->drawText(40, option.rect.y() + 18, QString::number(data.itemType->id));
  painter->drawPixmap(4, option.rect.y(), data.pixmap);
  // painter->drawLine(0, option.rect.y() + 19, option.rect.width(), option.rect.y() + 19);
}

QSize Delegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
  return QSize(0, 40);
}