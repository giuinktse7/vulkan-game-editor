#pragma once

#include <QTabWidget>
#include <QTabBar>
#include <QSvgWidget>
#include <QFrame>
#include <QString>
#include <QObject>
#include <QVariant>
#include <QMimeData>

#include <QProxyStyle>

#include <memory>

#include <optional>
#include "qt_util.h"

#include "map_view_widget.h"
#include "../time_point.h"

class QWidget;
class QMouseEvent;
class QSize;
class QPropertyAnimation;

class OpacityAnimation : public QObject
{
  Q_OBJECT
public:
  struct AnimationData
  {
    int duration = 3000;
    double startValue = 0.0;
    double endValue = 1.0;

    // AnimationData(const AnimationData &other) = default;
    // AnimationData &operator=(const AnimationData &other) = default;
  };

  OpacityAnimation();
  OpacityAnimation(QWidget *widget);

  AnimationData forward;
  AnimationData backward;

  void showWidget();
  void hideWidget();

signals:
  void preShow();
  void postShow();

private:
  enum class AnimationState
  {
    Showing,
    Hiding,
    None
  };

  bool internalStop = false;

  AnimationState animationState = AnimationState::None;

  QWidget *widget;
  QPropertyAnimation *animation;
};

class MapTabWidget : public QTabWidget
{
  Q_OBJECT

  class MapTabBar : public QTabBar
  {
  public:
    MapTabBar(MapTabWidget *parent);

    void setCloseButtonVisible(int tabIndex, bool visible);

    int tabAt(const QPoint &pos) const;

    QSize minimumSizeHint() const override;

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void paintEvent(QPaintEvent *event) override;

  private:
    std::optional<QPoint> dragStartPosition;
    int closePendingIndex = -1;

    QtScrollBar *scrollBar;
    QtUtil::ScrollState tabBarScrollState;
    OpacityAnimation scrollBarAnimation;
    struct ScrollVisibilityState
    {
      bool hasTimer = false;
      TimePoint newTimerStart;
    } scrollVisibilityState;

    int scrollOffset = 0;

    QImage *closeButtonImage;

    /*
      A value of -1 means no tab is hovered.
    */
    int hoveredIndex = -1;
    int prevActiveIndex = -1;

    std::optional<int> dragHoverIndex;

    /*
      True if this widget has been shown at least once.
    */
    bool hasBeenShown = false;

    void removedTabEvent(int removedIndex);

    void setHoveredIndex(int index);
    void setDragHoveredIndex(int index);

    void updateScrollBarVisibility();

    QPoint mapToScrolled(const QPoint pos) const;

    QWidget *getActiveWidget();

    inline bool pressed() const
    {
      return dragStartPosition.has_value();
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

    void createScrollVisibilityTimer(time_t millis);
    bool tabOverflow() const;
  };

public:
  MapTabWidget(QWidget *parent = nullptr);

  QSize minimumSizeHint() const override;

  int addTabWithButton(QWidget *widget, const QString &text, QVariant data = QVariant());

signals:
  void mapTabAdded(int index);

protected:
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

/*
  Defines the available MimeData for a drag & drop operation on a map tab.
*/
class MapTabMimeData : public QMimeData
{
public:
  MapTabMimeData();

  static const QString integerMimeType()
  {
    static const QString mimeType = "vulkan-game-editor-mimetype:int";
    return mimeType;
  }

  void setInt(int value);
  int getInt() const;

  bool hasFormat(const QString &mimeType) const override;
  QStringList formats() const override;

  QVariant retrieveData(const QString &mimeType, QVariant::Type type) const override;
};

class TestProxyStyle : public QProxyStyle
{
public:
  void drawControl(ControlElement element,
                   const QStyleOption *option,
                   QPainter *painter,
                   const QWidget *widget) const override;
};
