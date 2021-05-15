#pragma once

#include <QAbstractListModel>
#include <QListView>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickView>
#include <QUrl>

#include <memory>
#include <optional>
#include <unordered_map>

#include "../../item.h"
#include "../../items.h"
#include "../../signal.h"
#include "../../tracked_item.h"
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

class PropertyWindowEventFilter : public QtUtil::EventFilter
{
  public:
    PropertyWindowEventFilter(ItemPropertyWindow *parent);

    bool eventFilter(QObject *obj, QEvent *event) override;

  private:
    ItemPropertyWindow *propertyWindow;
};

class QmlApplicationContext : public QObject
{
    Q_OBJECT
  public:
    explicit QmlApplicationContext(QObject *parent = 0)
        : QObject(parent) {}
    Q_INVOKABLE void setCursor(Qt::CursorShape cursor)
    {
        QApplication::setOverrideCursor(cursor);
    }

    Q_INVOKABLE void resetCursor()
    {
        QApplication::restoreOverrideCursor();
    }
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
    void countChanged(ItemLocation &itemLocation, int count, bool shouldCommit = false);

  public:
    ItemPropertyWindow(QUrl filepath, MainWindow *mainWindow);

    void startContainerItemDrag(PropertiesUI::ContainerNode *treeNode, int index);
    bool itemDropEvent(PropertiesUI::ContainerNode *treeNode, int index, const ItemDrag::DraggableItem *droppedItem);
    bool containerItemSelectedEvent(PropertiesUI::ContainerNode *treeNode, int index);

    Q_INVOKABLE void setPropertyItemCount(int count, bool shouldCommit = false);
    Q_INVOKABLE int getPropertyItemItemid() const;

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
            : trackedItem(item), tileIndex(tileIndex), latestCommittedCount(item->count())
        {
            DEBUG_ASSERT(!item->isContainer(), "Item may not be a container. Use FocusedContainer instead.");
        }

        FocusedItem(const FocusedItem &other) = default;
        FocusedItem &operator=(const FocusedItem &other) = default;
        FocusedItem(FocusedItem &&other) = default;

        Item *item() const noexcept
        {
            return trackedItem.item();
        }

        TrackedItem trackedItem;
        size_t tileIndex;

        uint8_t latestCommittedCount;
    };

    struct FocusedContainer
    {
        FocusedContainer(Item *item, size_t tileIndex)
            : trackedContainer(item), _trackedItem(item), tileIndex(tileIndex)
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

        TrackedItem &trackedItem() noexcept
        {
            return _trackedItem;
        }

        void setTrackedItem(Item *item)
        {
            if (_trackedItem.item() != item)
            {
                _trackedItem = TrackedItem(item);
            }
        }

        size_t tileIndex;
        TrackedContainer trackedContainer;
        TrackedItem _trackedItem;
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

        TrackedItem trackedGround;
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
    void setContainerVisible(bool visible);

    void setFocused(FocusedGround &&ground);
    void setFocused(FocusedItem &&item);
    void setFocused(FocusedContainer &&container);

    void setPropertyItem(Item *item);

    /**
   * Returns a child from QML with objectName : name
   */
    inline QObject *child(const char *name);

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

        std::optional<TrackedItem> propertyItem()
        {
            return _propertyItem;
        }

        const std::optional<TrackedItem> propertyItem() const
        {
            return _propertyItem;
        }

        void setPropertyItem(Item *item)
        {
            if (_propertyItem.has_value() && _propertyItem->item() == item)
            {
                return;
            }

            _propertyItem = TrackedItem(item);
        }

        MapView *mapView;

        Position selectedPosition;

      private:
        std::variant<std::monostate, FocusedItem, FocusedGround, FocusedContainer> _focusedItem;

        // This is the tracked item that is used for the property window (excluding container contents)
        std::optional<TrackedItem> _propertyItem;
    };

    State state;

    std::optional<ItemDrag::DragOperation> dragOperation;
};

inline QObject *ItemPropertyWindow::child(const char *name)
{
    return rootObject()->findChild<QObject *>(name);
}

// Images
class ItemTypeImageProvider : public QQuickImageProvider
{
  public:
    ItemTypeImageProvider()
        : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};

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
