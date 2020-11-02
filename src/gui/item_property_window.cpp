#include "item_property_window.h"

#include <QWidget>

ItemPropertyWindow::ItemPropertyWindow(QString path)
{
  setSource(QUrl::fromLocalFile(path));
  auto container = QWidget::createWindowContainer(this);
  container->setMinimumWidth(200);
}

QWidget *ItemPropertyWindow::wrapInWidget(QWidget *parent)
{
  QWidget *wrapper = QWidget::createWindowContainer(this, parent);
  wrapper->setObjectName("ItemPropertyWindow wrapper");

  return wrapper;
}