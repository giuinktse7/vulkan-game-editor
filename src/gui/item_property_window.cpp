#include "item_property_window.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QWidget>

namespace ObjectName
{
  constexpr auto CountSpinBox = "count_spinbox";
  constexpr auto ActionIdSpinBox = "action_id_spinbox";
  constexpr auto UniqueIdSpinBox = "unique_id_spinbox";
} // namespace ObjectName

ItemPropertyWindow::ItemPropertyWindow(QUrl url)
    : _url(url)
{
  engine()->rootContext()->setContextProperty("propertyWindow", this);
  setSource(_url);
}

void ItemPropertyWindow::setItem(const Item &item)
{
  setCount(item.count());
}

void ItemPropertyWindow::setCount(uint8_t count)
{
  auto countSpinBox = child(ObjectName::CountSpinBox);
  countSpinBox->setProperty("value", count);
}

QWidget *ItemPropertyWindow::wrapInWidget(QWidget *parent)
{
  QWidget *wrapper = QWidget::createWindowContainer(this, parent);
  wrapper->setObjectName("ItemPropertyWindow wrapper");

  return wrapper;
}

void ItemPropertyWindow::reloadSource()
{
  VME_LOG_D("ItemPropertyWindow source reloaded.");
  engine()->clearComponentCache();
  setSource(QUrl::fromLocalFile("../resources/qml/itemPropertyWindow.qml"));
}
}
