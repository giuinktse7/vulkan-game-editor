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
    void countChanged(int count);

  public:
    ItemPropertyWindow(QUrl url, MainWindow *mainWindow);

    void startContainerItemDrag(PropertiesUI::ContainerNode *treeNode, int index);
    bool itemDropEvent(PropertiesUI::ContainerNode *treeNode, int index, const ItemDrag::DraggableItem *droppedItem);

    QWidget *wrapInWidget(QWidget *parent = nullptr);

    void reloadSource();

    void refresh();

    void setMapView(MapView &mapView);
    void resetMapView();

    void focusItem(Item *item, Position &position, MapView &mapView);
    void focusGround(Position &position, MapView &mapView);
    void resetFocus();

    QWidget *wrapperWidget() const noexcept;

  protected:
    bool event(QEvent *event);
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

  private:
    friend class PropertyWindowEventFilter;
    void setCount(uint8_t count);
    void setContainerVisible(bool visible);

    /**
   * Returns a child from QML with objectName : name
   */
    inline QObject *child(const char *name);

    QUrl _url;
    MainWindow *mainWindow;
    QWidget *_wrapperWidget;

    PropertiesUI::ContainerTree containerTree;

    struct FocusedItem
    {
        FocusedItem(Position position, Item *item, size_t tileIndex)
            : position(position), trackedItem(item), tileIndex(tileIndex) {}

        FocusedItem(const FocusedItem &other) = default;
        FocusedItem &operator=(const FocusedItem &other) = default;
        FocusedItem(FocusedItem &&other) = default;

        Item *item() const noexcept
        {
            return trackedItem.item();
        }

        Position position;
        TrackedItem trackedItem;
        size_t tileIndex;
    };

    struct FocusedGround
    {
        FocusedGround(Position position, Item *ground)
            : position(position), trackedGround(ground) {}

        FocusedGround(const FocusedGround &other) = default;
        FocusedGround &operator=(const FocusedGround &other) = default;
        FocusedGround(FocusedGround &&other) = default;

        Item *item() const noexcept
        {
            return trackedGround.item();
        }

        Position position;
        TrackedItem trackedGround;
    };

    struct State
    {
        MapView *mapView;
        std::variant<std::monostate, FocusedItem, FocusedGround> focusedItem;

        template <typename T>
        bool holds();

        template <typename T>
        T &focusedAs();
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
    static_assert(util::is_one_of<T, FocusedItem, FocusedGround>::value, "T must be FocusedItem or FocusedGround");

    return std::holds_alternative<T>(focusedItem);
}

template <typename T>
T &ItemPropertyWindow::State::focusedAs()
{
    static_assert(util::is_one_of<T, FocusedItem, FocusedGround>::value, "T must be FocusedItem or FocusedGround");

    return std::get<T>(focusedItem);
}
