#include "item_palette_model.h"

#include "core/item_palette.h"
#include "core/tileset.h"
#include "gui_thing_image.h"
#include "item_palette_store.h"

ItemPaletteModel::ItemPaletteModel()
    : itemPalettes(ItemPalettes::getItemPaletteList())
{
}

int ItemPaletteModel::rowCount(const QModelIndex &) const
{
    return itemPalettes.size();
}

QVariant ItemPaletteModel::data(const QModelIndex &modelIndex, int role) const
{
    switch (role)
    {
        case Role::Text:
        {
            uint32_t index = modelIndex.row();
            return QString::fromStdString(itemPalettes.at(index)->name());
        }
        
        case Role::Value:
        {
            uint32_t index = modelIndex.row();
            return QString::fromStdString(itemPalettes.at(index)->id());
        }
        default:
            break;
    }

    return QVariant();
}

void ItemPaletteModel::indexClicked(int index)
{
    // TODO
    VME_LOG_D("ItemPaletteModel::indexClicked: " << index);
}

QHash<int, QByteArray> ItemPaletteModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[to_underlying(Role::Text)] = "text";
    roles[to_underlying(Role::Value)] = "value";

    return roles;
}

#include "moc_item_palette_model.cpp"