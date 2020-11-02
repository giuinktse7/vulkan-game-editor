#pragma once

#include <QQuickView>
#include <QString>

class ItemPropertyWindow : QQuickView
{
public:
  ItemPropertyWindow(QString path);

  QWidget *wrapInWidget(QWidget *parent = nullptr);
};