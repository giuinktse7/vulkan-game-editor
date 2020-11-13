#include "item_list.h"

#include "../items.h"
#include "../logger.h"

#include "qt_util.h"

ItemList::ItemList(QWidget *parent) : QListView(parent)
{
  setAlternatingRowColors(true);

  _model = new QtItemTypeModel(this);
  setModel(_model);
}

void ItemList::addItem(uint32_t serverId)
{
  _model->addItem(serverId);
}

void ItemList::addItems(std::vector<uint32_t> &&serverIds)
{
  _model->addItems(std::move(serverIds));
}

void ItemList::addItems(uint32_t from, uint32_t to)
{
  _model->addItems(from, to);
}

ItemTypeModelItem ItemList::itemAtIndex(QModelIndex index)
{
  QVariant variant = _model->data(index);
  return variant.value<ItemTypeModelItem>();
}

ItemTypeModelItem ItemTypeModelItem::fromServerId(uint32_t serverId)
{
  ItemTypeModelItem value;
  value.itemType = Items::items.getItemTypeByServerId(serverId);
  value.pixmap = QtUtil::itemPixmap(serverId);

  return value;
}

void QtItemTypeModel::populate(std::vector<ItemTypeModelItem> &&items)
{
  beginResetModel();
  _data = std::move(items);
  endResetModel();
}

int QtItemTypeModel::rowCount(const QModelIndex &parent) const
{
  return static_cast<int>(_data.size());
}

void QtItemTypeModel::addItem(uint32_t serverId)
{
  if (Items::items.validItemType(serverId))
  {
    int size = static_cast<int>(_data.size());
    beginInsertRows(QModelIndex(), size, size + 1);
    _data.emplace_back(ItemTypeModelItem::fromServerId(serverId));
    endInsertRows();
  }
}
void QtItemTypeModel::addItems(uint32_t from, uint32_t to)
{
  DEBUG_ASSERT(from < to, "Bad range.");

  std::vector<uint32_t> ids;

  // Assume that most IDs in the range are valid
  ids.reserve(to - from);

  for (uint32_t serverId = from; serverId < to; ++serverId)
  {
    if (Items::items.validItemType(serverId))
    {
      ids.emplace_back(serverId);
    }
  }

  addItems(std::move(ids));
}

void QtItemTypeModel::addItems(std::vector<uint32_t> &&serverIds)
{
  std::remove_if(serverIds.begin(), serverIds.end(), [](uint32_t serverId) {
    return !Items::items.validItemType(serverId);
  });

  int size = static_cast<int>(_data.size());
  beginInsertRows(QModelIndex(), size, size + serverIds.size());
  for (const auto serverId : serverIds)
  {
    _data.emplace_back(ItemTypeModelItem::fromServerId(serverId));
  }
  endInsertRows();
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