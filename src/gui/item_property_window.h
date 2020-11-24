#pragma once

#include <QAbstractListModel>
#include <QListView>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickView>
#include <QUrl>

#include <optional>

#include "../item.h"
#include "../util.h"
#include "draggable_item.h"
#include "qt_util.h"

namespace GuiItemContainer
{
  class ItemModel;
}
class MainWindow;

class PropertyWindowEventFilter : public QtUtil::EventFilter
{
public:
  PropertyWindowEventFilter(QObject *parent) : QtUtil::EventFilter(parent) {}
  bool eventFilter(QObject *obj, QEvent *event) override;
};

class QmlApplicationContext : public QObject
{
  Q_OBJECT
public:
  explicit QmlApplicationContext(QObject *parent = 0) : QObject(parent) {}
  Q_INVOKABLE void setCursor(Qt::CursorShape cursor)
  {
    QApplication::setOverrideCursor(cursor);
  }

  Q_INVOKABLE void resetCursor()
  {
    QApplication::restoreOverrideCursor();
  }
};

class ItemPropertyWindow : public QQuickView
{
  Q_OBJECT
signals:
  void countChanged(int count);

public:
  ItemPropertyWindow(QUrl url, MainWindow *mainWindow);

  Q_INVOKABLE bool itemDropEvent(int index, QByteArray serializedMapItem);
  Q_INVOKABLE bool testDropEvent(QByteArray serializedMapItem);

  Q_INVOKABLE void startContainerItemDrag(int index);

  QWidget *wrapInWidget(QWidget *parent = nullptr);

  void reloadSource();

  void refresh();

  void setMapView(MapView &mapView);
  void resetMapView();

  void focusItem(Item &item, Position &position, MapView &mapView);
  void focusGround(Position &position, MapView &mapView);
  void resetFocus();

private:
  void setCount(uint8_t count);
  void setContainerVisible(bool visible);

  /**
   * Returns a child from QML with objectName : name
   */
  inline QObject *child(const char *name);

  QUrl _url;
  MainWindow *mainWindow;

  GuiItemContainer::ItemModel *itemContainerModel;

  struct FocusedItem
  {
    Position position;
    Item *item;
    size_t tileIndex;
  };

  struct FocusedGround
  {
    Position position;
    Item *ground;
  };

  struct State
  {
    MapView *mapView;
    std::variant<std::monostate, FocusedItem, FocusedGround> focusedItem;

    template <typename T>
    bool holds();

    template <typename T>
    T focusedAs();
  };

  State state;

  std::optional<ItemDrag::DragOperation> dragOperation;
};

inline QObject *ItemPropertyWindow::child(const char *name)
{
  return rootObject()->findChild<QObject *>(name);
}

/**
 * Makes it possible to mix connect syntax like:
 *   QmlBind::connect(countSpinbox, SIGNAL(valueChanged()), [=]{...});
 */
class QmlBind : public QObject
{
  using F = std::function<void()>;
  Q_OBJECT
  F f;

public:
  Q_SLOT void call() { f(); }
  static QMetaObject::Connection connect(
      QObject *sender,
      const char *signal,
      F &&f)
  {
    if (!sender)
      return {};

    return QObject::connect(sender, signal, new QmlBind(std::move(f), sender), SLOT(call()));
  }

private:
  QmlBind(F &&f, QObject *parent = {}) : QObject(parent),
                                         f(std::move(f)) {}
};

// Images
class ItemTypeImageProvider : public QQuickImageProvider
{
public:
  ItemTypeImageProvider()
      : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

  QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
  {
    bool success;
    auto serverId = id.toInt(&success);
    if (!success)
    {
      QPixmap pixmap(32, 32);
      pixmap.fill(QColor("black").rgba());
      return pixmap;
    }

    return QtUtil::itemPixmap(serverId);
  }
};

namespace GuiItemContainer
{
  // class ItemWrapper
  // {
  // public:
  //   ItemWrapper(Item *item);

  //   int serverId() const;

  //   bool hasItem() const noexcept;

  // private:
  //   Item *item;
  // };

  class ItemModel : public QAbstractListModel
  {
    Q_OBJECT
    Q_PROPERTY(int capacity READ capacity NOTIFY capacityChanged)

  public:
    enum ItemModelRole
    {
      ServerIdRole = Qt::UserRole + 1
    };

    ItemModel(QObject *parent = 0);

    bool addItem(Item &&item);
    void setContainer(ContainerItem container);
    void reset();
    int capacity();

    void refresh();

    ContainerItem *containerItem()
    {
      return _container ? &_container.value() : nullptr;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  signals:
    void capacityChanged(int capacity);

  protected:
    QHash<int, QByteArray> roleNames() const;

  private:
    std::optional<ContainerItem> _container;
  };
} // namespace GuiItemContainer

template <typename T>
bool ItemPropertyWindow::State::holds()
{
  static_assert(util::is_one_of<T, FocusedItem, FocusedGround>::value, "T must be FocusedItem or FocusedGround");

  return std::holds_alternative<T>(focusedItem);
}

template <typename T>
T ItemPropertyWindow::State::focusedAs()
{
  static_assert(util::is_one_of<T, FocusedItem, FocusedGround>::value, "T must be FocusedItem or FocusedGround");

  return std::get<T>(focusedItem);
}