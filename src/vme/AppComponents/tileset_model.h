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

  public:
    TileSetModel();
    TileSetModel(std::shared_ptr<Tileset> tileset);

    // Signals
    template <auto MemberFunction, typename T>
    void onSelectBrush(T *instance);

    Q_INVOKABLE void indexClicked(int index);

    enum Roles
    {
        ImageUriRole = Qt::UserRole + 1
    };

    void setTileset(std::shared_ptr<Tileset> &&tileset);
    void clear();

    int rowCount(const QModelIndex & = QModelIndex()) const override;

    int columnCount(const QModelIndex & = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    std::shared_ptr<Tileset> _tileset = nullptr;

  private:
    // Signals
    Nano::Signal<void(Brush *)> selectBrush;
};

template <auto MemberFunction, typename T>
void TileSetModel::onSelectBrush(T *instance)
{
    selectBrush.connect<MemberFunction>(instance);
}
