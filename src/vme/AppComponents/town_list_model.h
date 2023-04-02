#pragma once

#include <QAbstractListModel>

#include <memory>

#include "core/map.h"
#include "core/position.h"
#include "core/town.h"

class QmlPosition : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(int y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(int z READ z WRITE setZ NOTIFY zChanged)

  public:
    QmlPosition(Position position);

    int x() const;
    int y() const;
    int z() const;

    void setX(int x);
    void setY(int y);
    void setZ(int z);

  signals:
    void xChanged(int value);
    void yChanged(int value);
    void zChanged(int value);

  private:
    Position _position;
};

class TownListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int size READ size NOTIFY sizeChanged)

  private:
    struct TownData
    {
        uint32_t id;
        std::unique_ptr<QmlPosition> templePos;
    };

  public:
    enum class Role
    {
        Name = Qt::UserRole + 1,
        ItemId = Qt::UserRole + 2,
        TemplePos = Qt::UserRole + 3,
    };

    TownListModel(std::shared_ptr<Map> map, QObject *parent = nullptr);
    TownListModel(QObject *parent = nullptr);

    TownData &get(int index)
    {
        return _data.at(index);
    }

    void addTown(const Town &town);

    void setMap(std::shared_ptr<Map> map);

    void clear();
    bool empty();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int size();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  signals:
    void sizeChanged(int size);

  protected:
    QHash<int, QByteArray> roleNames() const;

  private:
    std::weak_ptr<Map> _map;
    std::vector<TownData> _data;
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