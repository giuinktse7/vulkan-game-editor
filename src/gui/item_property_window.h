#pragma once

#include <QAbstractListModel>
#include <QListView>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickView>
#include <QUrl>

#include <memory>
#include <optional>

#include "../item.h"
#include "../util.h"
#include "draggable_item.h"
#include "qt_util.h"

namespace GuiItemContainer
{
  class ContainerModel;
  class ItemModel;
} // namespace GuiItemContainer

class MainWindow;
class ItemPropertyWindow;

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

  // std::vector<std::unique_ptr<GuiItemContainer::ItemModel>> itemContainerModels;
  std::unique_ptr<GuiItemContainer::ContainerModel> containerModel;

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

// Images
class ItemTypeImageProvider : public QQuickImageProvider
{
public:
  ItemTypeImageProvider()
      : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

  QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};

namespace GuiItemContainer
{
  class ContainerModel : public QAbstractListModel
  {
    Q_OBJECT
    Q_PROPERTY(int size READ size NOTIFY sizeChanged)

  public:
    enum class Role
    {
      ItemModel = Qt::UserRole + 1
    };

    ContainerModel(QObject *parent = 0);

    void clear();
    void refresh(int index);

    void addContainerItem(ContainerItem containerItem);
    void remove(int index);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int size();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  signals:
    void sizeChanged(int size);

  protected:
    QHash<int, QByteArray> roleNames() const;

  private:
    std::vector<std::unique_ptr<ItemModel>> itemModels;
  };

  class ItemModel : public QAbstractListModel
  {
    Q_OBJECT
    Q_PROPERTY(int capacity READ capacity NOTIFY capacityChanged)
    Q_PROPERTY(int size READ size)

  public:
    enum ItemModelRole
    {
      ServerIdRole = Qt::UserRole + 1
    };

    ItemModel(QObject *parent = 0);
    ItemModel(ContainerItem containerItem, QObject *parent = 0);

    bool addItem(Item &&item);
    void setContainer(ContainerItem container);
    void reset();
    int capacity();
    int size();

    void refresh();

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