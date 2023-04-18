#pragma once

#include <QAbstractListModel>

#include <memory>

#include "core/map.h"
#include "core/position.h"
#include "core/town.h"
#include <tuple>

class TownData : public QObject
{
  public:
    Q_OBJECT
    Q_PROPERTY(int id MEMBER _id)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(int z READ z WRITE setZ NOTIFY zChanged)

  public:
    TownData(uint32_t id, QString name, QObject *parent = nullptr);
    TownData(const TownData &);

    ~TownData();

    QString name()
    {
        return _name;
    }

    int x() const
    {
        return _x;
    }

    int y() const
    {
        return _y;
    }

    int z() const
    {
        return _z;
    }

    Position templePos()
    {
        return Position(_x, _y, _z);
    }

    void setName(const QString &name);
    void setX(const int x);
    void setY(const int y);
    void setZ(const int z);

  signals:
    void nameChanged(const QString &newName);
    void xChanged(const int x);
    void yChanged(const int y);
    void zChanged(const int z);

  public:
    uint32_t _id;
    QString _name;
    int _x;
    int _y;
    int _z;
};

class TownListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int size READ size NOTIFY sizeChanged)

  public:
    enum class Role
    {
        Name = Qt::UserRole + 1,
        ItemId = Qt::UserRole + 2,
        TempleX = Qt::UserRole + 3,
        TempleY = Qt::UserRole + 4,
        TempleZ = Qt::UserRole + 5,
    };

    Q_INVOKABLE void nameChanged(QString text, int index);
    Q_INVOKABLE void xChanged(int value, int index);
    Q_INVOKABLE void yChanged(int value, int index);
    Q_INVOKABLE void zChanged(int value, int index);

    TownListModel(std::shared_ptr<Map> map, QObject *parent = nullptr);
    TownListModel(QObject *parent = nullptr);

    Q_INVOKABLE TownData *get(int index);

    void addTown(const Town &town);

    void setMap(std::shared_ptr<Map> map);

    void clear();
    bool empty();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int size();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);

  signals:
    void sizeChanged(int size);

  protected:
    QHash<int, QByteArray> roleNames() const;

  private:
    std::weak_ptr<Map> _map;
    std::vector<std::unique_ptr<TownData>> _data;
};
