#include "town_list_model.h"

#include <QQmlEngine>
#include <QVariant>
#include <ranges>

QmlPosition::QmlPosition(Position position)
    : _position(position) {}

QmlPosition::QmlPosition(const QmlPosition &other)
{
    _position = other._position;
}

QmlPosition &QmlPosition::operator=(const QmlPosition &other)
{
    _position = other._position;
    return *this;
}

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

void TownData::setName(const QString &name)
{
    VME_LOG_D("TownData::setName: " << name.toStdString());
    if (name != _name)
    {
        _name = name;
        emit nameChanged(name);
    }
}

void TownData::setTemplePos(const QmlPosition &position)
{
    if (_templePos != position)
    {
        _templePos = position;
        emit templePosChanged(position);
    }
}

TownData::TownData(uint32_t id, QString name, QObject *parent)
    // : _id(id), _name(name), _templePos(std::make_unique<QmlPosition>(Position(14, 14, 7))) {}
    : QObject(parent), _id(id), _name(name), _templePos(Position(1000, 1000, 7))
{
}

TownData::TownData(const TownData &other)
    : QObject(other.parent()), _id(other._id), _name(other._name), _templePos(other._templePos.position())
{
    VME_LOG_D("TownData::TownData(const TownData &other), name: " << _name.toStdString());
}

TownListModel::TownListModel(QObject *parent)
{
}

TownListModel::TownListModel(std::shared_ptr<Map> map, QObject *parent)
    : _map(map), QAbstractListModel(parent)
{
    for (const auto &value : map->towns() | std::views::values)
    {
        _data.push_back(std::make_unique<TownData>(value.id(), "Untitled", this));
    }
}

TownData *TownListModel::get(int index)
{
    if (index < 0 || index >= _data.size())
    {
        return nullptr;
    }

    auto ptr = _data.at(index).get();

    // Ensure that the object is owned by C++ and not by QML
    if (QQmlEngine::objectOwnership(ptr) != QQmlEngine::ObjectOwnership::CppOwnership)
    {
        QQmlEngine::setObjectOwnership(ptr, QQmlEngine::CppOwnership);
    }

    return ptr;
}

TownData::~TownData()
{
    VME_LOG_D("TownData::~TownData: " << _name.toStdString());
}

void TownListModel::addTown(const Town &town)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _data.push_back(std::make_unique<TownData>(town.id(), "Untitled", this));
    endInsertRows();
}

QVariant TownListModel::data(const QModelIndex &modelIndex, int role) const
{
    auto index = modelIndex.row();
    if (index < 0 || index >= rowCount())
        return QVariant();

    const TownData *item = _data.at(index).get();

    if (!item)
    {
        VME_LOG_D("TownListModel::data: item is null for index " << index);
        return QVariant();
    }

    if (role == to_underlying(Role::Name))
    {
        return item->_name;
    }
    else if (role == to_underlying(Role::ItemId))
    {
        return QVariant::fromValue(item->_id);
    }
    else if (role == to_underlying(Role::TemplePos))
    {
        return &item->_templePos;
    }

    return QVariant();
}

bool TownListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
    {
        return false;
    }

    if (role == to_underlying(Role::Name))
    {
        TownData *item = _data.at(index.row()).get();
        item->setName(value.toString());
        emit dataChanged(index, index, {to_underlying(Role::Name)});
        return true;
    }
    else if (role == to_underlying(Role::ItemId))
    {
        // TODO
        return false;
    }
    else if (role == to_underlying(Role::TemplePos))
    {
        // TODO
        return false;
    }
}

void TownListModel::nameChanged(QString text, int index)
{
    VME_LOG_D("TownListModel::nameChanged");
    if (auto m = _map.lock())
    {
        TownData *modelTown = _data.at(index).get();
        if (!modelTown)
        {
            ABORT_PROGRAM("TownListModel::nameChanged: modelTown is null for index " << index);
        }

        Town *town = m->getTown(modelTown->_id);
        if (town)
        {
            town->setName(text.toStdString());

            auto modelIndex = createIndex(index, 0);
            emit dataChanged(modelIndex, modelIndex, {to_underlying(Role::Name)});
        }
    }
}

void TownListModel::xChanged(int value, int index)
{
    if (auto m = _map.lock())
    {
        Town *town = m->getTown(_data.at(index)->_id);
        if (town)
        {
            Position pos = town->templePosition();
            pos.x = value;
            town->setTemplePosition(pos);

            auto modelIndex = createIndex(index, 0);
            emit dataChanged(modelIndex, modelIndex, {to_underlying(Role::TemplePos)});
        }
    }
}

void TownListModel::yChanged(int value, int index)
{
}

void TownListModel::zChanged(int value, int index)
{
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
    VME_LOG_D("TownListModel::setMap");
    _map = map;

    beginResetModel();
    _data.clear();
    for (const auto &value : map->towns() | std::views::values)
    {
        _data.push_back(std::make_unique<TownData>(value.id(), QString::fromStdString(value.name()), this));
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
