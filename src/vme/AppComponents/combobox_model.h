#pragma once

#include <QAbstractListModel>
#include <memory>
#include <qqml.h>

#include "core/signal.h"
#include "core/util.h"

class Tileset;
class ItemPalette;

class ComboBoxModel : public QAbstractListModel
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

    ComboBoxModel();
    ComboBoxModel(std::vector<NamedId> items);

    Q_INVOKABLE void setData(std::vector<NamedId> items);
    // Q_INVOKABLE QString getValue(int index);

    int rowCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<NamedId> items;
};
