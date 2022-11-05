#include "container_list_model.h"

#include "../../debug.h"
#include "../../item.h"
#include "../../util.h"
#include "container_model.h"
#include "property_container_tree.h"

using ContainerListModel = PropertiesUI::ContainerListModel;
using UIContainerModel = PropertiesUI::UIContainerModel;

ContainerListModel::ContainerListModel(QObject *parent)
    : QAbstractListModel(parent) {}

std::vector<UIContainerModel *>::iterator ContainerListModel::find(const UIContainerModel *model)
{
    return std::find_if(
        itemModels.begin(),
        itemModels.end(),
        [model](const UIContainerModel *_model) { return model == _model; });
}

void ContainerListModel::addItemModel(UIContainerModel *model)
{
    VME_LOG_D("addItemModel " << model->containerItem()->name());
    auto modelSize = static_cast<int>(itemModels.size());

    beginInsertRows(QModelIndex(), modelSize, modelSize);
    itemModels.emplace_back(model);
    endInsertRows();

    emit sizeChanged(size());
}

void ContainerListModel::remove(UIContainerModel *model)
{
    auto found = std::find_if(itemModels.begin(),
                              itemModels.end(),
                              [model](const UIContainerModel *_model) { return model == _model; });

    if (found == itemModels.end())
    {
        VME_LOG_D("ContainerModel::remove: ItemModel '" << model << "' was not present.");
        return;
    }

    int index = static_cast<int>(found - itemModels.begin());
    beginRemoveRows(QModelIndex(), index, index);
    itemModels.erase(found);
    endRemoveRows();

    emit sizeChanged(size());
}

void ContainerListModel::refresh(UIContainerModel *model)
{
    auto found = find(model);
    DEBUG_ASSERT(found != itemModels.end(), "model was not present.");

    (*found)->refresh();
}

void ContainerListModel::closeContainer(int containerIndex)
{
    itemModels.at(containerIndex)->node()->close();
}

void ContainerListModel::remove(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    itemModels.erase(itemModels.begin() + index);
    endRemoveRows();
    emit sizeChanged(size());
}

int ContainerListModel::rowCount(const QModelIndex &parent) const
{
    return static_cast<int>(itemModels.size());
}

int ContainerListModel::size()
{
    return rowCount();
}

QVariant ContainerListModel::data(const QModelIndex &modelIndex, int role) const
{
    auto index = modelIndex.row();
    if (index < 0 || index >= rowCount())
        return QVariant();

    if (role == to_underlying(Role::ItemModel))
    {
        return QVariant::fromValue(itemModels.at(index));
    }

    return QVariant();
}

void ContainerListModel::clear()
{
    if (itemModels.empty())
        return;

    beginResetModel();
    itemModels.clear();
    endResetModel();
    emit sizeChanged(size());
}

void ContainerListModel::refresh(int index)
{
    itemModels.at(index)->refresh();
    auto modelIndex = createIndex(index, 0);
    dataChanged(modelIndex, modelIndex);
}

void ContainerListModel::refreshAll()
{
    for (auto &itemModel : itemModels)
    {
        // TODO Maybe check if the container is visible here? Hidden containers do not need to be refreshed.
        itemModel->refresh();
    }
}

QHash<int, QByteArray> ContainerListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[to_underlying(Role::ItemModel)] = "itemModel";

    return roles;
}
