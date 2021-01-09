#pragma once

#include <QAbstractItemDelegate>
#include <QAbstractListModel>
#include <QPixmap>

class ItemType;

namespace ItemPaletteUI
{
    struct ModelItem
    {
        ItemType *itemType;
        QPixmap pixmap;

        static ModelItem fromServerId(uint32_t serverId);
    };

    class TilesetModel : public QAbstractListModel
    {
        Q_OBJECT
      public:
        TilesetModel(QObject *parent = nullptr)
            : QAbstractListModel(parent) {}

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        void addItem(uint32_t serverId);
        void addItems(std::vector<uint32_t> &&serverIds);
        void addItems(uint32_t from, uint32_t to);

        void populate(std::vector<ModelItem> &&items);

        std::vector<ModelItem> _data;
    };

    class ItemDelegate : public QAbstractItemDelegate
    {
        Q_OBJECT
      public:
        explicit ItemDelegate(QObject *parent = nullptr);

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    };
} // namespace ItemPaletteUI

// Q_DECLARE_METATYPE(ItemPaletteUI);
