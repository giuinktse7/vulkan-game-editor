#pragma once

#include <QAbstractListModel>
#include <QListView>
#include <QQuickImageProvider>
#include <QQuickItem>
#include <QQuickView>
#include <QUrl>

#include "../item.h"
#include "qt_util.h"

class PropertyWindowEventFilter : public QtUtil::EventFilter
{
public:
  PropertyWindowEventFilter(QObject *parent) : QtUtil::EventFilter(parent) {}
  bool eventFilter(QObject *obj, QEvent *event) override;
};

namespace QtItemModel
{
  class Model;
}

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
  ItemPropertyWindow(QUrl url);

  QWidget *wrapInWidget(QWidget *parent = nullptr);

  void reloadSource();

  void setItem(const Item &item);
  void resetItem();

private:
  QUrl _url;
  QtItemModel::Model *model;

  void setCount(uint8_t count);
  void setContainerVisible(bool visible);

  /**
   * Returns a child from QML with objectName : name
   */
  inline QObject *child(const char *name);
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

namespace QtItemTest
{
  class ItemWrapper
  {
  public:
    ItemWrapper(Item *item);

    int serverId() const;

    bool hasItem() const noexcept;

  private:
    Item *item;
  };

  class ItemModel : public QAbstractListModel
  {
    Q_OBJECT
  public:
    enum AnimalRoles
    {
      ServerIdRole = Qt::UserRole + 1
    };

    ItemModel(QObject *parent = 0);

    void addItem(const ItemWrapper &item);
    void setContainer(ContainerItem &container);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  protected:
    QHash<int, QByteArray> roleNames() const;

  private:
    std::vector<ItemWrapper> _itemWrappers;
  };
} // namespace QtItemTest