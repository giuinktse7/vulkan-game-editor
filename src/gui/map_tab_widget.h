#pragma once

#include <QTabWidget>
#include <QTabBar>

class QMouseEvent;

class MapTabWidget : public QTabWidget
{
  class MapTabBar : public QTabBar
  {
  protected:
    void mousePressEvent(QMouseEvent *event);
  };
};