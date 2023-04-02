#include "town_list_model.h"

#include <QVariant>
#include <ranges>

QmlPosition::QmlPosition(Position position)
    : _position(position) {}

int QmlPosition::x() const
{
    return _position.x;
}

int QmlPosition::y() const
{
    return _position.y;
}

int QmlPosition::z() const
{
    return _position.z;
}

void QmlPosition::setX(int x)
{
    if (x != _position.x)
    {
        _position.x = x;
        emit xChanged(x);
    }
}

void QmlPosition::setY(int y)
{
    if (y != _position.y)
    {
        _position.y = y;
        emit yChanged(y);
    }
}

void QmlPosition::setZ(int z)
{
    if (z != _position.z)
    {
        _position.z = z;
        emit zChanged(z);
    }
}

TownListModel::TownListModel(QObject *parent)
{
}

TownListModel::TownListModel(std::shared_ptr<Map> map, QObject *parent)
    : _map(map), QAbstractListModel(parent)
{
    for (const auto &value : map->towns() | std::views::values)
    {
        _data.push_back(TownData{.id = value.id(), .templePos = std::make_unique<QmlPosition>(value.templePosition())});
    }
}

void TownListModel::addTown(const Town &town)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _data.push_back(TownData{.id = town.id(), .templePos = std::make_unique<QmlPosition>(town.templePosition())});
    endInsertRows();
}

QVariant TownListModel::data(const QModelIndex &modelIndex, int role) const
{
    auto index = modelIndex.row();
    if (index < 0 || index >= rowCount())
        return QVariant();

    const TownData &item = _data.at(index);
    if (auto m = _map.lock())
    {
        Town *town = m->getTown(item.id);

        if (role == to_underlying(Role::Name))
        {
            if (town)
            {
                return QVariant::fromValue(QString::fromStdString(town->name()));
            }
            else
            {
                return QVariant::fromValue(QString(""));
            }
        }
        else if (role == to_underlying(Role::ItemId))
        {
            return QVariant::fromValue(item.id);
        }
        else if (role == to_underlying(Role::TemplePos))
        {
            return QVariant::fromValue(item.templePos.get());
        }
    }

    return QVariant();
}

QHash<int, QByteArray> TownListModel::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[to_underlying(Role::Name)] = "name";
    roles[to_underlying(Role::ItemId)] = "itemId";
    roles[to_underlying(Role::TemplePos)] = "templePos";

    return roles;
}

void TownListModel::setMap(std::shared_ptr<Map> map)
{
    _map = map;

    beginResetModel();
    _data.clear();
    for (const auto &value : map->towns() | std::views::values)
    {
        _data.push_back(TownData{.id = value.id(), .templePos = std::make_unique<QmlPosition>(value.templePosition())});
    }
    endResetModel();
}

int TownListModel::size()
{
    return rowCount();
}

bool TownListModel::empty()
{
    return size() == 0;
}

void TownListModel::clear()
{
    beginResetModel();
    _map.reset();
    _data.clear();
    endResetModel();
}

int TownListModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(_data.size());
}
