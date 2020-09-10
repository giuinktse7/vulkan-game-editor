#pragma once

#include <QVulkanWindow>
#include <QMenu>
#include <memory>

#include "../util.h"

#include "../map_renderer.h"
#include "../trianglerenderer.h"
#include "../map_view.h"

QT_BEGIN_NAMESPACE
class QWidget;
class QPoint;
class util::Size;
QT_END_NAMESPACE

class VulkanWindow : public QVulkanWindow
{
  Q_OBJECT
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

  VulkanWindow(std::shared_ptr<MapView> mapView);

  QVulkanWindowRenderer *createRenderer() override;

  MapView *getMapView() const;

  QWidget *wrapInWidget(QWidget *parent = nullptr);
  void lostFocus();

  void showContextMenu(QPoint position);
  void closeContextMenu();

  QRect localGeometry() const;

  util::Size vulkanSwapChainImageSize() const;
  glm::mat4 projectionMatrix();

  QWidget *widget;

signals:
  void scrollEvent(int degrees);
  void mousePosEvent(util::Point<double> mousePos);

private:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  void onVisibilityChanged(QWindow::Visibility visibility);

  // std::unique_ptr<TriangleRenderer> renderer;
  MapRenderer *renderer;
  std::shared_ptr<MapView> mapView;
  ContextMenu *contextMenu;

  // Holds the current scroll amount. (see wheelEvent)
  int scrollAngleBuffer;
};
