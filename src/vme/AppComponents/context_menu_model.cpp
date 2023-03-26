#include "context_menu_model.h"

#include "core/item_palette.h"
#include "core/tileset.h"
#include "gui_thing_image.h"

ContextMenuModel::ContextMenuModel() {}

ContextMenuModel::ContextMenuModel(std::vector<ContextMenuItem> items)
    : items(items) {}

int ContextMenuModel::rowCount(const QModelIndex &) const
{
    return items.size();
}

QVariant ContextMenuModel::data(const QModelIndex &modelIndex, int role) const
{
    switch (role)
    {
        case Role::Option:
        {
            uint32_t index = modelIndex.row();
            return items.at(index).option;
        }
        case Role::Text:
        {
            uint32_t index = modelIndex.row();
            return QString::fromStdString(items.at(index).text);
        }
        case Role::Enabled:
        {
            uint32_t index = modelIndex.row();
            return QVariant::fromValue(items.at(index).enabled);
        }
        default:
            break;
    }

    return QVariant();
}

void ContextMenuModel::setData(std::vector<ContextMenuItem> items)
{
    beginResetModel();
    this->items = items;
    endResetModel();
}

QHash<int, QByteArray> ContextMenuModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[to_underlying(Role::Option)] = "option";
    roles[to_underlying(Role::Text)] = "itemText";
    roles[to_underlying(Role::Enabled)] = "enabled";

    return roles;
}

void clear();

// #include "context_menu_model.cpp"
