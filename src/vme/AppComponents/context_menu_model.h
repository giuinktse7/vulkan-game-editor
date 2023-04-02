#pragma once

#include <QAbstractListModel>
#include <memory>
#include <qqml.h>

#include "core/signal.h"
#include "core/util.h"

class Tileset;
class ItemPalette;

enum class ContextMenuOption
{
    Copy,
    Paste,
    Cut,
    Delete,
    CopyPosition,
    SelectRawBrush,
    SelectGroundBrush,
    SelectDoodadBrush,
    SelectMountainBrush,
    SelectWallBrush,
    SelectBorderBrush,
    SelectCreatureBrush
};

class ContextMenuModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_MINOR_VERSION(1)

  public:
    enum Role
    {
        Option = Qt::UserRole + 1,
        Text = Qt::UserRole + 2,
        Enabled = Qt::UserRole + 3,
    };

    struct ContextMenuItem
    {
        std::string text;
        /**
         * @brief Corresponds to the value of a ContextMenuOption
         */
        int option;
        bool enabled;
    };

    ContextMenuModel();
    ContextMenuModel(std::vector<ContextMenuItem> items);

    Q_INVOKABLE void setData(std::vector<ContextMenuItem> items);
    // Q_INVOKABLE QString getValue(int index);

    int rowCount(const QModelIndex & = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<ContextMenuItem> items;
};
