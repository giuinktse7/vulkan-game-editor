#pragma once

#include <QTabWidget>
#include <QTabBar>

#include <optional>

class QWidget;
class QMouseEvent;
class QString;

class MapTabWidget : public QTabWidget
{
  class MapTabBar : public QTabBar
  {
  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

  private:
    QWidget *getActiveWidget();

    std::optional<QPoint> activePressPos;
    bool closePending = false;

    inline bool pressed() const
    {
      return activePressPos.has_value();
    }

    /*
      pos A position relative to the TabBar
    */
    bool intersectsCloseButton(QPoint pos);

    MapTabWidget *parentWidget() const;
  };

public:
  MapTabWidget(QWidget *parent = nullptr);

  int addTabWithButton(QWidget *widget, const QString &text);

private:
  void closeTab(int index);
};