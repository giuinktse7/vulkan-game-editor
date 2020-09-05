#pragma once

#include <QVulkanWindow>
#include <QMenu>
#include <memory>

#include "../map_renderer.h"
#include "../trianglerenderer.h"
#include "../map_view.h"

QT_BEGIN_NAMESPACE
class QWidget;
class QPoint;
QT_END_NAMESPACE

class VulkanWindow : public QVulkanWindow
{
public:
  class ContextMenu : public QMenu
  {
  public:
    ContextMenu(VulkanWindow *window, QWidget *widget);

    bool event(QEvent *e) override;
    QRect relativeGeometry() const;
    QRect localGeometry() const;

  protected:
    void mousePressEvent(QMouseEvent *event) override;

  private:
    VulkanWindow *window;

    bool selfClicked(QPoint pos) const;
  };

  VulkanWindow(MapView *mapView);

  QVulkanWindowRenderer *createRenderer() override;

  MapView *getMapView() const;

  QWidget *wrapInWidget();
  void lostFocus();

  void showContextMenu(QPoint position);
  void closeContextMenu();

  QRect localGeometry() const;

  QWidget *widget;

protected:
private:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

  // std::unique_ptr<TriangleRenderer> renderer;
  MapRenderer *renderer;
  MapView *mapView;
  ContextMenu *contextMenu;
};
