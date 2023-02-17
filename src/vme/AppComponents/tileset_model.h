#pragma once

#include <QAbstractTableModel>
#include <memory>
#include <qqml.h>

class Tileset;

class TileSetModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

    Q_PROPERTY(int columnCount READ columnCount NOTIFY columnCountChanged)

  public:
    TileSetModel(std::shared_ptr<Tileset> tileset);

    enum Roles
    {
        ImageUriRole = Qt::UserRole + 1
    };

    int rowCount(const QModelIndex & = QModelIndex()) const override;

    int columnCount(const QModelIndex & = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    QHash<int, QByteArray> roleNames() const override;

    std::shared_ptr<Tileset> _tileset = nullptr;

    void setColumnCount(int columnCount)
    {
        _columnCount = columnCount;
        emit columnCountChanged(columnCount);
    }

  signals:
    void columnCountChanged(int);

  private:
    int _columnCount = 4;
};