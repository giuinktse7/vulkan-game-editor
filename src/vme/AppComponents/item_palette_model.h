#pragma once

#include <QAbstractListModel>
#include <memory>
#include <qqml.h>

#include "core/signal.h"

class Tileset;
class ItemPalette;

class ItemPaletteModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

  public:
    enum Role
    {
        Text = Qt::UserRole + 1,
        Value = Qt::UserRole + 2
    };

    ItemPaletteModel();

    int rowCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    template <auto MemberFunction, typename T>
    void onSelectPalette(T *instance);

    Q_INVOKABLE void indexClicked(int index);

  private:
    std::vector<std::shared_ptr<ItemPalette>> itemPalettes;

    // Signals
    Nano::Signal<void(ItemPalette *)> selectPalette;
};

template <auto MemberFunction, typename T>
void ItemPaletteModel::onSelectPalette(T *instance)
{
    selectPalette.connect<MemberFunction>(instance);
}
