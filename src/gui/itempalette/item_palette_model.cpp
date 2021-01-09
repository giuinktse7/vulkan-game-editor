#include "item_palette_model.h"

#include <QPainter>

#include "../../debug.h"
#include "../../items.h"
#include "../qt_util.h"

using TilesetModel = ItemPaletteUI::TilesetModel;
using ModelItem = ItemPaletteUI::ModelItem;
using ItemDelegate = ItemPaletteUI::ItemDelegate;

ModelItem ModelItem::fromServerId(uint32_t serverId)
{
    ModelItem value;
    value.itemType = Items::items.getItemTypeByServerId(serverId);
    value.pixmap = QtUtil::itemPixmap(serverId);

    return value;
}

void TilesetModel::populate(std::vector<ModelItem> &&items)
{
    beginResetModel();
    _data = std::move(items);
    endResetModel();
}

int TilesetModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(_data.size());
}

void TilesetModel::addItem(uint32_t serverId)
{
    if (Items::items.validItemType(serverId))
    {
        int size = static_cast<int>(_data.size());
        beginInsertRows(QModelIndex(), size, size + 1);
        _data.emplace_back(ModelItem::fromServerId(serverId));
        endInsertRows();
    }
}
void TilesetModel::addItems(uint32_t from, uint32_t to)
{
    DEBUG_ASSERT(from < to, "Bad range.");

    std::vector<uint32_t> ids;

    // Assume that most IDs in the range are valid
    ids.reserve(to - from);

    for (uint32_t serverId = from; serverId < to; ++serverId)
    {
        // TODO Include corpses. Filtering out corpses is temporary for testing.
        // if (Items::items.validItemType(serverId))
        if (Items::items.validItemType(serverId) && !Items::items.getItemTypeByServerId(serverId)->isCorpse())
        // if (Items::items.validItemType(serverId) && Items::items.getItemTypeByServerId(serverId)->isGroundTile())
        {
            ids.emplace_back(serverId);
        }
    }

    ids.shrink_to_fit();
    VME_LOG("Added " << ids.size() << " items");
    addItems(std::move(ids));
}

void TilesetModel::addItems(std::vector<uint32_t> &&serverIds)
{
    serverIds.erase(std::remove_if(serverIds.begin(),
                                   serverIds.end(),
                                   [](uint32_t serverId) {
                                       return !Items::items.validItemType(serverId);
                                   }),
                    serverIds.end());

    int size = static_cast<int>(_data.size());
    beginInsertRows(QModelIndex(), size, size + static_cast<int>(serverIds.size()));
    for (const auto serverId : serverIds)
    {
        _data.emplace_back(ModelItem::fromServerId(serverId));
    }
    endInsertRows();
}

QVariant TilesetModel::data(const QModelIndex &index, int role) const
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

ItemDelegate::ItemDelegate(QObject *parent)
    : QAbstractItemDelegate(parent) {}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    ModelItem data = qvariant_cast<ModelItem>(index.data());

    // TODO Add option for rendering as a list with names
    // painter->drawText(40, option.rect.y() + 18, QString::number(data.itemType->id));
    // painter->drawPixmap(4, option.rect.y(), data.pixmap);
    // qDebug() << "Delegate::paint: " << option.rect;
    painter->drawPixmap(option.rect.x(), option.rect.y(), data.pixmap);
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(32, 32);
}