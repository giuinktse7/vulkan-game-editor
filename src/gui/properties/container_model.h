#pragma once

#include <QAbstractListModel>
#include <QString>

class Item;
struct Container;

namespace PropertiesUI
{
    struct ContainerNode;

    class UIContainerModel : public QAbstractListModel
    {
        Q_OBJECT
        Q_PROPERTY(int capacity READ capacity NOTIFY capacityChanged)
        Q_PROPERTY(int size READ size)
        Q_PROPERTY(int containerServerId READ containerServerId CONSTANT)
        Q_PROPERTY(QString containerName READ containerName CONSTANT)

      public:
        Q_INVOKABLE void containerItemLeftClicked(int index);
        Q_INVOKABLE void containerItemRightClicked(int index);
        Q_INVOKABLE bool itemDropEvent(int index, QByteArray serializedDraggableItem);
        Q_INVOKABLE void itemDragStartEvent(int index);

        enum ContainerModelRole
        {
            ServerIdRole = Qt::UserRole + 1,
            SubtypeRole = Qt::UserRole + 2
        };

        UIContainerModel(ContainerNode *treeNode, QObject *parent = 0);

        bool addItem(Item &&item);
        void reset();
        int capacity();
        int size();
        int containerServerId();
        QString containerName();

        Container *container() noexcept;
        Container *container() const noexcept;
        Item *containerItem() const noexcept;

        void refresh();
        void indexChanged(int index);

        int rowCount(const QModelIndex &parent = QModelIndex()) const;

        QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const;

        ContainerNode *node();

      signals:
        void capacityChanged(int capacity);

      protected:
        QHash<int, QByteArray> roleNames() const;

      private:
        ContainerNode *treeNode;
    };
} // namespace PropertiesUI
