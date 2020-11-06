#pragma once

#include <QQuickItem>
#include <QQuickView>
#include <QUrl>

#include "../item.h"

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

private:
  QUrl _url;

  void setCount(uint8_t count);

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