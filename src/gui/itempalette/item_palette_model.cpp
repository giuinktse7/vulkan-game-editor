#include "item_palette_model.h"

#include <QPainter>

#include "../../debug.h"
#include "../../items.h"
#include "../../tileset.h"
#include "../qt_util.h"

using TilesetModel = ItemPaletteUI::TilesetModel;
// using ModelItem = ItemPaletteUI::ModelItem;
using ItemDelegate = ItemPaletteUI::ItemDelegate;

void TilesetModel::setTileset(Tileset *tileset)
{
    beginResetModel();
    _tileset = tileset;
    endResetModel();
}

int TilesetModel::rowCount(const QModelIndex &parent) const
{
    return _tileset ? static_cast<int>(_tileset->size()) : 0;
}

Brush *TilesetModel::brushAtIndex(size_t index) const
{
    return _tileset ? _tileset->get(index) : nullptr;
}

Tileset *TilesetModel::tileset() const noexcept
{
    return _tileset;
}

void TilesetModel::clear()
{
    setTileset(nullptr);
}

// void TilesetModel::addItem(uint32_t serverId)
// {
//     if (Items::items.validItemType(serverId))
//     {
//         int size = static_cast<int>(_data.size());
//         beginInsertRows(QModelIndex(), size, size + 1);
//         _data.emplace_back(ModelItem::fromServerId(serverId));
//         endInsertRows();
//     }
// }

// void TilesetModel::addItems(uint32_t from, uint32_t to)
// {
//     DEBUG_ASSERT(from < to, "Bad range.");

//     std::vector<uint32_t> ids;

//     // Assume that most IDs in the range are valid
//     ids.reserve(to - from);

//     for (uint32_t serverId = from; serverId < to; ++serverId)
//     {
//         // TODO Include corpses. Filtering out corpses is temporary for testing.
//         // if (Items::items.validItemType(serverId))
//         if (Items::items.validItemType(serverId) && !Items::items.getItemTypeByServerId(serverId)->isCorpse())
//         // if (Items::items.validItemType(serverId) && Items::items.getItemTypeByServerId(serverId)->isGroundTile())
//         {
//             ids.emplace_back(serverId);
//         }
//     }

//     ids.shrink_to_fit();
//     VME_LOG("Added " << ids.size() << " items");
//     addItems(std::move(ids));
// }

// void TilesetModel::addItems(std::vector<uint32_t> &&serverIds)
// {
//     serverIds.erase(std::remove_if(serverIds.begin(),
//                                    serverIds.end(),
//                                    [](uint32_t serverId) {
//                                        return !Items::items.validItemType(serverId);
//                                    }),
//                     serverIds.end());

//     int size = static_cast<int>(_data.size());
//     beginInsertRows(QModelIndex(), size, size + static_cast<int>(serverIds.size()));
//     for (const auto serverId : serverIds)
//     {
//         _data.emplace_back(ModelItem::fromServerId(serverId));
//     }
//     endInsertRows();
// }

QVariant TilesetModel::data(const QModelIndex &index, int role) const
{
    // VME_LOG_D("data(" << index.row() << ", " << role << ")");
    if (index.row() < 0 || index.row() >= _tileset->size())
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        Brush *brush = _tileset->get(index.row());
        return GuiImageCache::get(brush->iconServerId());
    }

    return QVariant();
}

ItemDelegate::ItemDelegate(QObject *parent)
    : QAbstractItemDelegate(parent) {}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // TODO Add option for rendering as a list with names
    // painter->drawText(40, option.rect.y() + 18, QString::number(data.itemType->id));
    // painter->drawPixmap(4, option.rect.y(), data.pixmap);
    // qDebug() << "Delegate::paint: " << option.rect;
    painter->drawPixmap(option.rect.x(), option.rect.y(), qvariant_cast<QPixmap>(index.data(Qt::DisplayRole)));
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(32, 32);
}