#include "container_model.h"

#include "../../debug.h"
#include "property_container_tree.h"

#include "../../item.h"

using UIContainerModel = PropertiesUI::UIContainerModel;
using ContainerNode = PropertiesUI::ContainerNode;

UIContainerModel::UIContainerModel(PropertiesUI::ContainerNode *treeNode, QObject *parent)
    : QAbstractListModel(parent), treeNode(treeNode)
{
    // TODO: Maybe this reset is not necessary?
    beginResetModel();
    endResetModel();
}

void UIContainerModel::refresh()
{
    beginResetModel();
    endResetModel();
}

ContainerNode *UIContainerModel::node()
{
    return treeNode;
}

void UIContainerModel::containerItemLeftClicked(int index)
{
    if (index >= size())
        return;

    VME_LOG_D("containerItemLeftClicked. Item id: " << containerServerId() << ", index: " << index);

    treeNode->itemLeftClickedEvent(index);
}

void UIContainerModel::containerItemRightClicked(int index)
{
    if (index >= size())
        return;

    VME_LOG_D("containerItemRightClicked. Item id: " << containerServerId() << ", index: " << index);

    if (container()->itemAt(index).isContainer())
    {
        treeNode->toggleChild(index);
    }
}

void UIContainerModel::itemDragStartEvent(int index)
{
    treeNode->itemDragStartEvent(index);
}

bool UIContainerModel::itemDropEvent(int index, QByteArray serializedDraggableItem)
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

    if (treeNode->isSelfOrParent(droppedItem->item()))
    {
        VME_LOG_D("Can not add an item to itself or to a sub-container of itself.");
        return false;
    }

    treeNode->itemDropEvent(index, droppedItem.get());
    return true;
}

int UIContainerModel::size()
{
    return static_cast<int>(treeNode->container()->size());
}

int UIContainerModel::containerServerId()
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

QString UIContainerModel::containerName()
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

int UIContainerModel::capacity()
{
    return static_cast<int>(treeNode->container()->capacity());
}

Item *UIContainerModel::containerItem() const noexcept
{
    return treeNode->containerItem();
}

Container *UIContainerModel::container() const noexcept
{
    return treeNode->container();
}

Container *UIContainerModel::container() noexcept
{
    return const_cast<Container *>(const_cast<const UIContainerModel *>(this)->container());
}

bool UIContainerModel::addItem(Item &&item)
{
    if (container()->isFull())
        return false;

    int size = static_cast<int>(container()->size());

    // ContainerModel::createIndex(size, 0);

    // beginInsertRows(QModelIndex(), size, size + 1);
    bool added = container()->addItem(std::move(item));
    // endInsertRows();

    emit dataChanged(UIContainerModel::createIndex(size, 0), UIContainerModel::createIndex(size + 1, 0));
    return added;
}

void UIContainerModel::indexChanged(int index)
{
    auto modelIndex = UIContainerModel::createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
}

int UIContainerModel::rowCount(const QModelIndex &parent) const
{
    return container()->capacity();
}

QVariant UIContainerModel::data(const QModelIndex &modelIndex, int role) const
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
    else if (role == SubtypeRole)
    {
        if (index >= container()->size())
        {
            return -1;
        }
        else
        {
            return container()->itemAt(index).subtype();
        }
    }

    return QVariant();
}

QHash<int, QByteArray> UIContainerModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ServerIdRole] = "serverId";
    roles[SubtypeRole] = "subtype";

    return roles;
}
