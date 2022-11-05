#include "thing_list_model.h"

int ThingListModel::rowCount(const QModelIndex &) const
{
    return 200;
}

int ThingListModel::columnCount(const QModelIndex &) const
{
    return 200;
}

QVariant ThingListModel::data(const QModelIndex &index, int role) const
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

QHash<int, QByteArray> ThingListModel::roleNames() const
{
    return {{Qt::DisplayRole, "display"}};
}