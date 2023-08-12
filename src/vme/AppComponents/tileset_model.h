#pragma once

#include <QAbstractTableModel>
#include <memory>
#include <qqml.h>

#include "core/signal.h"

class Tileset;
class Brush;

class TileSetModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

  signals:
    void brushIndexSelected(int index);

  public:
    TileSetModel();

    // Signals
    template <auto MemberFunction, typename T>
    void onSelectBrush(T *instance);

    Brush *getBrush(int index);
    /**
     * @brief Returns the index of the brush in the tileset.
     * @return The index of the brush in the tileset.
     * @note If the brush is not found, -1 is returned.
     */
    int indexOfBrush(Brush *brush);

    Q_INVOKABLE void indexClicked(int index);
    Q_INVOKABLE void clear();

    enum Roles
    {
        ImageUriRole = Qt::UserRole + 1
    };

    void setTileset(Tileset *tileset);

    int rowCount(const QModelIndex & = QModelIndex()) const override;

    int columnCount(const QModelIndex & = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    Tileset *_tileset = nullptr;

  private:
    // Signals
    Nano::Signal<void(Brush *)> selectBrush;
};

template <auto MemberFunction, typename T>
void TileSetModel::onSelectBrush(T *instance)
{
    selectBrush.connect<MemberFunction>(instance);
}
