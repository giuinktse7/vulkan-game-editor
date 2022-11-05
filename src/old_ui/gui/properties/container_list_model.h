#pragma once

#include <QAbstractListModel>

#include "container_model.h"

namespace PropertiesUI
{
    /**
     * A list model for QML that holds ContainerModel pointers.
     */
    class ContainerListModel : public QAbstractListModel
    {
        Q_OBJECT
      public:
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_INVOKABLE void closeContainer(int containerIndex);

      public:
        enum class Role
        {
            ItemModel = Qt::UserRole + 1
        };

        ContainerListModel(QObject *parent = 0);

        void clear();
        void refresh(int index);
        void refresh(UIContainerModel *model);
        void refreshAll();

        void addItemModel(UIContainerModel *model);
        void remove(UIContainerModel *model);
        void remove(int index);

        std::vector<UIContainerModel *>::iterator find(const UIContainerModel *model);

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int size();

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

      signals:
        void sizeChanged(int size);

      protected:
        QHash<int, QByteArray> roleNames() const;

      private:
        std::vector<UIContainerModel *> itemModels;
    };
} // namespace PropertiesUI
