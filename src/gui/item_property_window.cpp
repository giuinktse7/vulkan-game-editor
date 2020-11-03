#include "item_property_window.h"

#include <QQmlEngine>
#include <QQmlProperty>
#include <QWidget>

namespace ObjectName
{
  constexpr auto CountSpinbox = "count_spinbox";
}

ItemPropertyWindow::ItemPropertyWindow(QString path)
    : _path(std::move(path))
{
  setSource(QUrl::fromLocalFile(_path));
  auto container = QWidget::createWindowContainer(this);

  auto countSpinbox = child(ObjectName::CountSpinbox);
  QmlBind::connect(countSpinbox, SIGNAL(valueChanged()), [=] {
    int count = countSpinbox->property("value").toInt();
    emit countChanged(count);
  });
}

void ItemPropertyWindow::setItem(const Item &item)
{
  setCount(item.count());
}

void ItemPropertyWindow::setCount(int count)
{
  auto countSpinbox = child(ObjectName::CountSpinbox);
  countSpinbox->setProperty("value", count);
}

QWidget *ItemPropertyWindow::wrapInWidget(QWidget *parent)
{
  QWidget *wrapper = QWidget::createWindowContainer(this, parent);
  wrapper->setObjectName("ItemPropertyWindow wrapper");

  return wrapper;
}

void ItemPropertyWindow::reloadSource()
{
  setSource(QUrl::fromLocalFile(_path));
}
