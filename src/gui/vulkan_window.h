#pragma once

#include <memory>
#include <unordered_set>

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
      frame->mouseAction = window.mapView->editorAction.action();
      frame->mouseHover = window.showPreviewCursor;

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

  VulkanWindow(std::shared_ptr<Map> map, EditorAction &editorAction);
  ~VulkanWindow();

  QtVulkanInfo vulkanInfo;
  QWidget *widget = nullptr;
  EditorAction &editorAction;
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

  inline static bool isInstance(const VulkanWindow *pointer)
  {
    return instances.find(pointer) != instances.end();
  }

signals:
  void scrollEvent(int degrees);
  void mousePosChanged(util::Point<float> mousePos);
  void keyPressedEvent(QKeyEvent *event);

protected:
  bool event(QEvent *ev) override;

private:
  /**
 		*	Keeps track of all VulkanWindow instances. This is necessary for QT to
		* validate VulkanWindow pointers in a QVariant.
		*
		* See:
		* QtUtil::associatedVulkanWindow
	*/
  static std::unordered_set<const VulkanWindow *> instances;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  void onVisibilityChanged(QWindow::Visibility visibility);
  std::unique_ptr<MapView> mapView;

  QVulkanWindowRenderer *renderer = nullptr;
  ContextMenu *contextMenu = nullptr;

  // Holds the current scroll amount. (see wheelEvent)
  int scrollAngleBuffer = 0;
};
