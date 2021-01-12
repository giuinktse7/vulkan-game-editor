#pragma once

#include <QAbstractItemDelegate>
#include <QAbstractListModel>
#include <QPixmap>

class ItemType;
class Tileset;
class Brush;

namespace ItemPaletteUI
{
    // struct ModelItem
    // {
    //     ItemType *itemType;
    //     QPixmap pixmap;

    //     static ModelItem fromServerId(uint32_t serverId);
    // };

    class TilesetModel : public QAbstractListModel
    {
        Q_OBJECT
      public:
        TilesetModel(QObject *parent = nullptr)
            : QAbstractListModel(parent) {}

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        void setTileset(Tileset *tileset);

        Tileset *tileset() const noexcept;

        void clear();

        Brush *brushAtIndex(size_t index) const;

      private:
        Tileset *_tileset = nullptr;
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
