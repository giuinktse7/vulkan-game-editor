#include "item_palette_model.h"

#include <QPainter>

#include "../../debug.h"
#include "../../items.h"
#include "../../tileset.h"
#include "../qt_util.h"

using TilesetModel = ItemPaletteUI::TilesetModel;
// using ModelItem = ItemPaletteUI::ModelItem;
using ItemDelegate = ItemPaletteUI::ItemDelegate;
using HighlightAnimation = ItemPaletteUI::HighlightAnimation;

TilesetModel::TilesetModel(QObject *parent)
    : QAbstractListModel(parent), highlightAnimation(this)
{
}

void TilesetModel::setTileset(Tileset *tileset)
{
    beginResetModel();
    _tileset = tileset;
    endResetModel();
}

void TilesetModel::highlightIndex(const QModelIndex &modelIndex)
{
    highlightAnimation.runOnIndex(modelIndex);
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

QVariant TilesetModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= _tileset->size())
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        Brush *brush = _tileset->get(index.row());
        // return GuiImageCache::get(brush->iconServerId());
        return QVariant::fromValue(QtUtil::itemImageData(brush->iconServerId()));
    }
    else if (role == TilesetModel::HighlightRole)
    {
        if (QPersistentModelIndex(index) == highlightAnimation.index)
        {
            return highlightAnimation.currentValue();
        }
    }

    return QVariant();
}

ItemDelegate::ItemDelegate(QObject *parent)
    : QAbstractItemDelegate(parent), color("#2196F3")
{
    highlightBorderPen.setWidth(4);
    setHighlightPenOpacity(1.0f);
}

void ItemDelegate::setHighlightPenOpacity(float opacity) const
{
    color.setAlphaF(opacity);
    highlightBorderPen.setColor(color);
}

void ItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // TODO Add option for rendering as a list with names
    // painter->drawText(40, option.rect.y() + 18, QString::number(data.itemType->id));
    // painter->drawPixmap(4, option.rect.y(), data.pixmap);
    // qDebug() << "Delegate::paint: " << option.rect;

    ItemImageData imageData = qvariant_cast<ItemImageData>(index.data(Qt::DisplayRole));

    // painter->drawPixmap(option.rect.x(), option.rect.y(), qvariant_cast<QPixmap>(index.data(Qt::DisplayRole)));
    if (imageData.rect.width() > 32 || imageData.rect.height() > 32)
    {
        painter->drawImage(option.rect.topLeft(), imageData.image->copy(imageData.rect).mirrored().scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    else
    {
        painter->drawImage(option.rect.topLeft(), imageData.image->copy(imageData.rect).mirrored());
    }

    bool ok;
    int highlightOpacity = index.data(TilesetModel::HighlightRole).toInt(&ok);
    if (ok && highlightOpacity > 0)
    {
        painter->save();
        setHighlightPenOpacity(highlightOpacity / 100.0f);
        painter->setPen(highlightBorderPen);

        painter->drawRect(QRect(option.rect.x(), option.rect.y(), 32, 32));
        painter->restore();
    }
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QSize(32, 32);
}

HighlightAnimation::HighlightAnimation(TilesetModel *model, QObject *parent)
    : model(model)
{
    setKeyValueAt(0, 0);
    setKeyValueAt(0.25f, 100);
    setKeyValueAt(0.5f, 0);
    setKeyValueAt(0.75f, 100);
    setKeyValueAt(1.0f, 0);

    // setStartValue(100);
    // setEndValue(0);

    setDuration(2000);
    connect(this, &HighlightAnimation::valueChanged, [=](const QVariant &value) {
        model->dataChanged(index, index, QList{TilesetModel::HighlightRole});
    });
}

void HighlightAnimation::runOnIndex(const QModelIndex &modelIndex)
{
    if (state() == State::Running)
    {
        stop();
    }
    index = QPersistentModelIndex(modelIndex);

    start();
}
