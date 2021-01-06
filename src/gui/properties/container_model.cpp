#include "container_model.h"

#include "../../debug.h"
#include "property_container_tree.h"

#include "../../item.h"

using ContainerModel = PropertiesUI::ContainerModel;
using ContainerNode = PropertiesUI::ContainerNode;

ContainerModel::ContainerModel(PropertiesUI::ContainerNode *treeNode, QObject *parent)
    : QAbstractListModel(parent), treeNode(treeNode)
{
    // TODO: Maybe this reset is not necessary?
    beginResetModel();
    endResetModel();
}

void ContainerModel::refresh()
{
    beginResetModel();
    endResetModel();
}

void ContainerModel::containerItemClicked(int index)
{
    if (index >= size())
        return;

    VME_LOG_D("containerItemClicked. Item id: " << containerServerId() << ", index: " << index);

    if (container()->itemAt(index).isContainer())
    {
        treeNode->toggleChild(index);
    }
}

void ContainerModel::itemDragStartEvent(int index)
{
    treeNode->itemDragStartEvent(index);
}

bool ContainerModel::itemDropEvent(int index, QByteArray serializedDraggableItem)
{
    VME_LOG_D("Index: " << index);
    auto droppedItem = ItemDrag::DraggableItem::deserialize(serializedDraggableItem);
    if (!droppedItem)
    {
        VME_LOG("[Warning]: Could not read DraggableItem from qml QByteArray.");
        return false;
    }

    // Only accept items that can be picked up
    if (!droppedItem->item()->itemType->hasFlag(AppearanceFlag::Take))
    {
        return false;
    }

    if (droppedItem->item() == container()->item())
    {
        VME_LOG_D("Can not add item to itself.");
        return false;
    }

    treeNode->itemDropEvent(index, droppedItem.get());
    return true;
}

int ContainerModel::size()
{
    return static_cast<int>(treeNode->container()->size());
}

int ContainerModel::containerServerId()
{
    if (treeNode)
    {
        auto item = treeNode->containerItem();
        if (item)
        {
            return static_cast<int>(item->serverId());
        }
    }

    return -1;
}

QString ContainerModel::containerName()
{
    if (treeNode)
    {
        auto item = treeNode->containerItem();
        if (item)
        {
            auto name = item->name();
            if (!name.empty())
            {
                return QString::fromStdString(name);
            }
        }
    }

    return "Unknown container";
}

int ContainerModel::capacity()
{
    return static_cast<int>(treeNode->container()->capacity());
}

Item *ContainerModel::containerItem() const noexcept
{
    return treeNode->containerItem();
}

Container *ContainerModel::container() const noexcept
{
    return treeNode->container();
}

Container *ContainerModel::container() noexcept
{
    return const_cast<Container *>(const_cast<const ContainerModel *>(this)->container());
}

bool ContainerModel::addItem(Item &&item)
{
    if (container()->isFull())
        return false;

    int size = static_cast<int>(container()->size());

    // ContainerModel::createIndex(size, 0);

    // beginInsertRows(QModelIndex(), size, size + 1);
    bool added = container()->addItem(std::move(item));
    // endInsertRows();

    emit dataChanged(ContainerModel::createIndex(size, 0), ContainerModel::createIndex(size + 1, 0));
    return added;
}

void ContainerModel::indexChanged(int index)
{
    auto modelIndex = ContainerModel::createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
}

int ContainerModel::rowCount(const QModelIndex &parent) const
{
    return container()->capacity();
}

QVariant ContainerModel::data(const QModelIndex &modelIndex, int role) const
{
    auto index = modelIndex.row();
    if (index < 0 || index >= rowCount())
        return QVariant();

    if (role == ServerIdRole)
    {
        if (index >= container()->size())
        {
            return -1;
        }
        else
        {
            return container()->itemAt(index).serverId();
        }
    }

    return QVariant();
}

QHash<int, QByteArray> ContainerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ServerIdRole] = "serverId";

    return roles;
}