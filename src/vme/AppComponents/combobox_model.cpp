#include "combobox_model.h"

#include "core/item_palette.h"
#include "core/tileset.h"
#include "gui_thing_image.h"

ComboBoxModel::ComboBoxModel() {}

ComboBoxModel::ComboBoxModel(std::vector<NamedId> items)
    : items(items) {}

int ComboBoxModel::rowCount(const QModelIndex &) const
{
    return items.size();
}

QVariant ComboBoxModel::data(const QModelIndex &modelIndex, int role) const
{
    switch (role)
    {
        case Role::Text:
        {
            uint32_t index = modelIndex.row();
            return QString::fromStdString(items.at(index).name);
        }

        case Role::Value:
        {
            uint32_t index = modelIndex.row();
            return QString::fromStdString(items.at(index).id);
        }
        default:
            break;
    }

    return QVariant();
}

void ComboBoxModel::setData(std::vector<NamedId> items)
{
    beginResetModel();
    this->items = items;
    endResetModel();
}

QHash<int, QByteArray> ComboBoxModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[to_underlying(Role::Text)] = "text";
    roles[to_underlying(Role::Value)] = "value";

    return roles;
}

void ComboBoxModel::setSelectedIndex(int index)
{
    emit selectedIndexChanged(index);
}

bool ComboBoxModel::setSelectedId(std::string id)
{
    auto found = std::find_if(items.begin(),
                              items.end(),
                              [id](const NamedId &namedId) { return id == namedId.id; });

    if (found == items.end())
    {
        return false;
    }

    int index = static_cast<int>(found - items.begin());

    setSelectedIndex(index);
    return true;
}

#include "moc_combobox_model.cpp"