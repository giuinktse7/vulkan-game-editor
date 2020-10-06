#pragma once

#include <memory>

#include <QVulkanWindow>
#include <QMenu>
#include <QApplication>

#include "../util.h"

#include "../map_view.h"
#include "../graphics/vulkan_helpers.h"

#include "../qt/qt_vulkan_info.h"

#include "../map_renderer.h"

class MainWindow;

QT_BEGIN_NAMESPACE
class QWidget;
class QPoint;
class QEvent;
QT_END_NAMESPACE

class VulkanWindow : public QVulkanWindow
{
  Q_OBJECT
public:
  class Renderer : public QVulkanWindowRenderer
  {
  public:
    Renderer(VulkanWindow &window);
    inline void initResources() override
    {
      renderer.initResources(window.colorFormat());
    };

    inline void initSwapChainResources() override
    {
      renderer.initSwapChainResources(window.vulkanSwapChainImageSize());
    };

    inline void releaseSwapChainResources() override
    {
      renderer.releaseSwapChainResources();
    };

    inline void releaseResources() override
    {
      renderer.releaseResources();
    };

    inline void startNextFrame() override
    {
      renderer.setCurrentFrame(window.currentFrame());
      auto frame = renderer.currentFrame();
      frame->currentFrameIndex = window.currentFrame();
      frame->commandBuffer = window.currentCommandBuffer();
      frame->frameBuffer = window.currentFramebuffer();
      frame->mouseAction = window.mapView->mapViewMouseAction.action();
      frame->mouseHover = window.showPreviewCursor;
      frame->projectionMatrix = window.projectionMatrix();

      renderer.startNextFrame();
    }

  private:
    VulkanWindow &window;
    MapRenderer renderer;
  };

  class ContextMenu : public QMenu
  {
  public:
    ContextMenu(VulkanWindow *window, QWidget *widget);

    QRect relativeGeometry() const;
    QRect localGeometry() const;

  protected:
    void mousePressEvent(QMouseEvent *event) override;

  private:
    bool selfClicked(QPoint pos) const;
  };

  VulkanWindow(std::shared_ptr<Map> map, MapViewMouseAction &mapViewMouseAction);

  QtVulkanInfo vulkanInfo;
  QWidget *widget = nullptr;
  MapViewMouseAction &mapViewMouseAction;
  bool showPreviewCursor = false;

  std::string debugName;

  QVulkanWindowRenderer *createRenderer() override;

  void updateVulkanInfo();

  MapView *getMapView() const;

  QWidget *wrapInWidget(QWidget *parent = nullptr);
  void lostFocus();

  void showContextMenu(QPoint position);
  void closeContextMenu();

  QRect localGeometry() const;

  util::Size vulkanSwapChainImageSize() const;
  glm::mat4 projectionMatrix();

signals:
  void scrollEvent(int degrees);
  void mousePosChanged(util::Point<float> mousePos);
  void keyPressedEvent(QKeyEvent *event);

protected:
  bool event(QEvent *ev) override;

private:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  void onVisibilityChanged(QWindow::Visibility visibility);

  std::unique_ptr<MapView> mapView;

  QVulkanWindowRenderer *renderer = nullptr;
  ContextMenu *contextMenu = nullptr;

  // Holds the current scroll amount. (see wheelEvent)
  int scrollAngleBuffer = 0;

  int xVar = -1;
};
