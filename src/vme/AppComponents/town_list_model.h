#pragma once

#include <QAbstractListModel>

#include <memory>

#include "core/map.h"
#include "core/position.h"
#include "core/town.h"
#include <tuple>

class QmlPosition : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(int z READ z WRITE setZ NOTIFY zChanged)

  public:
    QmlPosition(Position position);
    QmlPosition(const QmlPosition &other);
    QmlPosition &operator=(const QmlPosition &other);
    int x() const;
    int y() const;
    int z() const;

    void setX(int x);
    void setY(int y);
    void setZ(int z);

    Position position() const noexcept
    {
        return _position;
    }

    auto operator<=>(const QmlPosition &other) const
    {
        int x1 = x();
        int y1 = y();
        int z1 = z();
        int x2 = other.x();
        int y2 = other.y();
        int z2 = other.z();
        return std::tie(x1, y1, z1) <=> std::tie(x2, y2, z2);
    }

    bool operator==(const QmlPosition &other) const
    {
        int x1 = x();
        int y1 = y();
        int z1 = z();
        int x2 = other.x();
        int y2 = other.y();
        int z2 = other.z();
        return std::tie(x1, y1, z1) == std::tie(x2, y2, z2);
    }

  signals:
    void xChanged(int value);
    void yChanged(int value);
    void zChanged(int value);

  private:
    Position _position;
};

class TownData : public QObject
{
  public:
    Q_OBJECT
    Q_PROPERTY(int id MEMBER _id)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QmlPosition templePos READ templePos WRITE setTemplePos NOTIFY templePosChanged)

  public:
    TownData(uint32_t id, QString name, QObject *parent = nullptr);
    TownData(const TownData &);

    ~TownData();

    QString name()
    {
        return _name;
    }

    QmlPosition templePos()
    {
        return _templePos;
    }

    void setName(const QString &name);
    void setTemplePos(const QmlPosition &position);

  signals:
    void nameChanged(const QString &newName);
    void templePosChanged(const QmlPosition &position);

  public:
    uint32_t _id;
    QString _name;
    QmlPosition _templePos;
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
        TemplePos = Qt::UserRole + 3,
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

// class QmlPosition : public QObject
// {
//     Q_OBJECT
//     Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
//     Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
//     Q_PROPERTY(int z READ z WRITE setZ NOTIFY zChanged)

//   public:
//     QmlPosition(Position position);

//     int x() const;
//     int y() const;
//     int z() const;

//     void setX(int x);
//     void setY(int y);
//     void setZ(int z);

//   signals:
//     void xChanged(int value);
//     void yChanged(int value);
//     void zChanged(int value);

//   private:
//     Position _position;
// };

// class TownItem : public QObject
// {
//     Q_OBJECT
//     Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

//   public:
//     QString name() const;
//     void setName(const QString &name);

//     TownItem(Town *town);

//     Town *town;
//     std::unique_ptr<QmlPosition> templePos;

//   signals:
//     void nameChanged(const QString &name);

//   private:
// };

// class TownListModel : public QAbstractListModel
// {
//     Q_OBJECT
//     Q_PROPERTY(int size READ size NOTIFY sizeChanged)

//   private:
//     struct TownData
//     {
//         Town *town;
//     };

//   public:
//     enum class Role
//     {
//         Name = Qt::UserRole + 1,
//         ItemId = Qt::UserRole + 2,
//         TemplePos = Qt::UserRole + 3,
//     };

//     TownListModel(QObject *parent = 0);

//     TownItem &get(int index)
//     {
//         return _data.at(index);
//     }

//     TownItem *getById(int id);

//     void clear();
//     bool empty();

//     int rowCount(const QModelIndex &parent = QModelIndex()) const;
//     int size();

//     QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

//   signals:
//     void sizeChanged(int size);

//   protected:
//     QHash<int, QByteArray> roleNames() const;

//   private:
//     std::vector<TownItem> _data;

//     uint32_t _nextId = 0;
// };