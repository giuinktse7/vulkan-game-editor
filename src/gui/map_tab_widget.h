#pragma once

#include <QTabWidget>
#include <QTabBar>
#include <QSvgWidget>
#include <QFrame>
#include <QString>
#include <QVariant>
#include <QMimeData>

#include <memory>

#include <optional>

class QWidget;
class QMouseEvent;

class MapTabWidget : public QTabWidget
{
  Q_OBJECT

  class MapTabBar : public QTabBar
  {
  public:
    MapTabBar(MapTabWidget *parent);

    void setCloseButtonVisible(int tabIndex, bool visible);

  protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

  private:
    std::optional<QPoint> dragStartPosition;
    int closePendingIndex = -1;
    /*
      A value of -1 means no tab is hovered.
    */
    int hoveredIndex = -1;
    int prevActiveIndex = -1;

    void removedTabEvent(int removedIndex);

    void setHoveredIndex(int index);

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

/*
  Defines the available MimeData for a drag & drop operation on a map tab.
*/
class MapTabMimeData : public QMimeData
{
public:
  MapTabMimeData();

  static QString TabIndexMimeType;

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