#include "thing_list_model.h"

int TileSetModel::rowCount(const QModelIndex &) const
{
    return 200;
}

int TileSetModel::columnCount(const QModelIndex &) const
{
    return 200;
}

QVariant TileSetModel::data(const QModelIndex &index, int role) const
{
    switch (role)
    {
        case Qt::DisplayRole:
            return QString("%1, %2").arg(index.column()).arg(index.row());
        default:
            break;
    }

    return QVariant();
}

QHash<int, QByteArray> TileSetModel::roleNames() const
{
    return {{Qt::DisplayRole, "display"}};
}