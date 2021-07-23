#pragma once

#include <QAbstractItemDelegate>
#include <QAbstractListModel>
#include <QPen>
#include <QPixmap>
#include <QVariantAnimation>

class ItemType;
class Tileset;
class Brush;
struct QtTextureArea;
class QPainter;
class QPoint;

namespace ItemPaletteUI
{
    class TilesetModel;

    class HighlightAnimation : public QVariantAnimation
    {
      public:
        HighlightAnimation(TilesetModel *model, QObject *parent = nullptr);

        void runOnIndex(const QModelIndex &modelIndex);

        QPersistentModelIndex index;

      private:
        void onValueChanged(const QVariant &value);

        TilesetModel *model;
    };

    class TilesetModel : public QAbstractListModel
    {
        Q_OBJECT
      public:
        static const int HighlightRole = Qt::UserRole + 1;
        static const int BrushRole = Qt::UserRole + 2;
        static const int CustomToolTipRole = Qt::UserRole + 3;

        TilesetModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        void setTileset(Tileset *tileset);

        void highlightIndex(const QModelIndex &modelIndex);

        Tileset *tileset() const noexcept;

        void clear();

        Brush *brushAtIndex(size_t index) const;

      private:
        Tileset *_tileset = nullptr;

        HighlightAnimation highlightAnimation;
    };

    class ItemDelegate : public QAbstractItemDelegate
    {
        Q_OBJECT
      public:
        explicit ItemDelegate(QObject *parent = nullptr);

        void paintTextureArea(QPainter *painter, const QPoint topLeft, const QtTextureArea &textureArea) const;

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

      private:
        void setHighlightPenOpacity(float opacity) const;
        mutable QPen highlightBorderPen;
        mutable QColor color;
    };

} // namespace ItemPaletteUI
