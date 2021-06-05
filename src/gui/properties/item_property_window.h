#pragma once

#include <QAbstractListModel>
#include <QListView>
#include <QQuickItem>
#include <QQuickView>
#include <QUrl>

#include <memory>
#include <optional>
#include <unordered_map>

#include "../../item.h"
#include "../../items.h"
#include "../../observable_item.h"
#include "../../signal.h"
#include "../../util.h"
#include "../draggable_item.h"
#include "../qt_util.h"
#include "property_container_tree.h"

class MainWindow;
class ItemPropertyWindow;
struct ItemLocation;

namespace PropertiesUI
{
    struct ContainerNode;

} // namespace PropertiesUI

class FluidTypeModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    enum ContainerModelRole
    {
        TextRole = Qt::UserRole + 1,
        SubtypeRole = Qt::UserRole + 2
    };

    FluidTypeModel(QObject *parent = 0);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &modelIndex, int role = Qt::DisplayRole) const;

  protected:
    QHash<int, QByteArray> roleNames() const;
};

class PropertyWindowEventFilter : public QtUtil::EventFilter
{
  public:
    PropertyWindowEventFilter(ItemPropertyWindow *parent);

    bool eventFilter(QObject *obj, QEvent *event) override;

  private:
    ItemPropertyWindow *propertyWindow;
};

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>ItemPropertyWindow>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class ItemPropertyWindow : public QQuickView
{
    Q_OBJECT
  signals:
    void textChanged(Item *item, const std::string &text);
    void subtypeChanged(Item *item, int subtype, bool shouldCommit);
    void actionIdChanged(Item *item, int actionId, bool shouldCommit);

  public:
    ItemPropertyWindow(QUrl filepath, MainWindow *mainWindow);

    void startContainerItemDrag(PropertiesUI::ContainerNode *treeNode, int index);
    bool itemDropEvent(PropertiesUI::ContainerNode *treeNode, int index, const ItemDrag::DraggableItem *droppedItem);
    bool containerItemSelectedEvent(PropertiesUI::ContainerNode *treeNode, int index);

    Q_INVOKABLE void setFluidType(int index);
    Q_INVOKABLE void fluidTypeHighlighted(int highlightedIndex);
    Q_INVOKABLE void setPropertyItemCount(int count, bool shouldCommit = false);
    Q_INVOKABLE void setPropertyItemActionId(int actionId, bool shouldCommit = false);
    Q_INVOKABLE void setPropertyItemText(QString text);
    Q_INVOKABLE QString getItemPixmapString(int serverId, int subtype) const;
    QString getItemPixmapString(const Item &item) const;

    QWidget *wrapInWidget(QWidget *parent = nullptr);

    void reloadSource();

    void refresh();

    void focusItem(Item *item, Position &position, MapView &mapView);
    void focusGround(Item *item, Position &position, MapView &mapView);
    void resetFocus();

    QWidget *wrapperWidget() const noexcept;

  protected:
    bool event(QEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

  private:
    struct FocusedItem
    {
        FocusedItem(Item *item, size_t tileIndex)
            : item(item), tileIndex(tileIndex)
        {
            DEBUG_ASSERT(!item->isContainer(), "Item may not be a container. Use FocusedContainer instead.");
        }

        FocusedItem(const FocusedItem &other) = default;
        FocusedItem &operator=(const FocusedItem &other) = default;
        FocusedItem(FocusedItem &&other) = default;

        Item *item;
        size_t tileIndex;
    };

    struct FocusedContainer
    {
        FocusedContainer(Item *item, size_t tileIndex)
            : trackedContainer(item), tileIndex(tileIndex)
        {
            DEBUG_ASSERT(item->isContainer(), "Item must be a container.");
        }

        FocusedContainer(const FocusedContainer &other) = default;
        FocusedContainer &operator=(const FocusedContainer &other) = default;
        FocusedContainer(FocusedContainer &&other) = default;

        Item *containerItem() const noexcept
        {
            return trackedContainer.item();
        }

        size_t tileIndex;
        TrackedContainer trackedContainer;
        Item *trackedItem;
    };

    struct FocusedGround
    {
        FocusedGround(Position position, Item *ground)
            : trackedGround(ground) {}

        FocusedGround(const FocusedGround &other) = default;
        FocusedGround &operator=(const FocusedGround &other) = default;
        FocusedGround(FocusedGround &&other) = default;

        Item *item() const noexcept
        {
            return trackedGround.item();
        }

        ObservableItem trackedGround;
    };

    friend class PropertyWindowEventFilter;

    void setMapView(MapView &mapView);
    void resetMapView();

    void setSelectedPosition(const Position &pos);
    void resetSelectedPosition();

    void setQmlObjectActive(QObject *qmlObject, bool enabled);

    void show();
    void hide();

    void setCount(uint8_t count);

    void setFocused(FocusedGround &&ground);
    void setFocused(FocusedItem &&item);
    void setFocused(FocusedContainer &&container);

    void setPropertyItem(Item *item);

    /**
   * Returns a child from QML with objectName : name
   */
    inline QObject *child(const char *name);

    struct LatestCommittedPropertyValues
    {
        uint8_t subtype;
        int actionId;
        int uniqueId;
    } latestCommittedPropertyValues;

    QUrl _filepath;
    MainWindow *mainWindow;
    QWidget *_wrapperWidget;

    PropertiesUI::ContainerTree containerTree;

    struct State
    {
        template <typename T>
        bool holds();

        template <typename T>
        T &focusedAs();

        const std::variant<std::monostate, FocusedItem, FocusedGround, FocusedContainer> &focusedItem() const
        {
            return _focusedItem;
        }

        // The setFocused methods should only be called from ItemPropertyWindow::setFocused
        void setFocused(FocusedGround &&ground);
        void setFocused(FocusedItem &&item);
        void setFocused(FocusedContainer &&container);

        void resetFocused();

        MapView *mapView;

        Position selectedPosition;

        Item *propertyItem = nullptr;

      private:
        std::variant<std::monostate, FocusedItem, FocusedGround, FocusedContainer> _focusedItem;

        // The item that is used for the property window (excluding for container contents)
    };

    State state;

    std::optional<ItemDrag::DragOperation> dragOperation;

    FluidTypeModel fluidTypeModel;
};

inline QObject *ItemPropertyWindow::child(const char *name)
{
    return rootObject()->findChild<QObject *>(name);
}

template <typename T>
bool ItemPropertyWindow::State::holds()
{
    static_assert(util::is_one_of<T, FocusedItem, FocusedGround, FocusedContainer>::value, "T must be FocusedItem or FocusedGround");

    return std::holds_alternative<T>(_focusedItem);
}

template <typename T>
T &ItemPropertyWindow::State::focusedAs()
{
    static_assert(util::is_one_of<T, FocusedItem, FocusedGround, FocusedContainer>::value, "T must be FocusedItem or FocusedGround");

    return std::get<T>(_focusedItem);
}