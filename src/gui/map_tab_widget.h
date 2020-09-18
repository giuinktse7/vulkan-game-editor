#pragma once

#include <QTabWidget>
#include <QTabBar>
#include <QSvgWidget>
#include <QFrame>
#include <QString>
#include <QVariant>

#include <memory>

#include <optional>

class QWidget;
class QMouseEvent;

class MapTabWidget : public QTabWidget
{
  class MapTabBar : public QTabBar
  {
  public:
    MapTabBar(QWidget *parent = nullptr);

    void setCloseButtonVisible(int tabIndex, bool visible);

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override;

  private:
    std::optional<QPoint> activePressPos;
    int closePendingIndex = -1;
    /*
      A value of -1 means no tab is hovered.
    */
    int hoveredIndex = -1;
    int prevActiveIndex = -1;

    QWidget *getActiveWidget();

    inline bool pressed() const
    {
      return activePressPos.has_value();
    }

    /*
      point A point relative to the TabBar
    */
    bool intersectsCloseButton(QPoint point) const;
    bool intersectsCloseButton(QPoint point, int tabIndex) const;

    /*
    relativePoint A point relative to the TabBar
    */
    bool withinWidget(QPoint relativePoint) const;

    MapTabWidget *parentWidget() const;
  };

public:
  MapTabWidget(QWidget *parent = nullptr);

  int addTabWithButton(QWidget *widget, const QString &text, QVariant data = QVariant());

signals:
  void mapTabClosed(int index, QVariant data);

private:
  void closeTab(int index);
};

/*
  Inherits QFrame because QFrame supports "The Box Model"
  @see https://doc.qt.io/qt-5/stylesheet-customizing.html#box-model
*/
class SvgWidget : public QFrame
{
  Q_OBJECT

public:
  SvgWidget(const QString &file, QWidget *parent = nullptr);

protected:
  bool event(QEvent *event) override;

private:
  QSvgWidget *svg;
};