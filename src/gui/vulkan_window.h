#pragma once

#include <QVulkanWindow>
#include <QMenu>
#include <memory>

#include "../util.h"

#include "../map_renderer.h"
#include "../trianglerenderer.h"
#include "../map_view.h"
#include "../graphics/vulkan_helpers.h"

class MainWindow;

QT_BEGIN_NAMESPACE
class QWidget;
class QPoint;
class QEvent;
QT_END_NAMESPACE

class VulkanInfoFromWindow : public VulkanInfo
{
public:
  VulkanInfoFromWindow();
  VulkanInfoFromWindow(VulkanWindow *window);

  inline VkDevice device() const override;
  inline VkPhysicalDevice physicalDevice() const override;
  inline VkCommandPool graphicsCommandPool() const override;
  inline VkQueue graphicsQueue() const override;
  inline uint32_t graphicsQueueFamilyIndex() const override;

  VulkanWindow *window;
};

class VulkanWindow : public QVulkanWindow
{
  Q_OBJECT
public:
  class ContextMenu : public QMenu
  {
  public:
    ContextMenu(VulkanWindow *window, QWidget *widget);

    QRect relativeGeometry() const;
    QRect localGeometry() const;

  protected:
    void mousePressEvent(QMouseEvent *event) override;

  private:
    VulkanWindow *window;

    bool selfClicked(QPoint pos) const;
  };

  VulkanWindow(std::unique_ptr<MapView> mapView, MapViewMouseAction &mapViewMouseAction);

  VulkanInfoFromWindow vulkanInfo;
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

  // std::unique_ptr<TriangleRenderer> renderer;
  MapRenderer *renderer = nullptr;
  ContextMenu *contextMenu = nullptr;

  // Holds the current scroll amount. (see wheelEvent)
  int scrollAngleBuffer = 0;
};

inline VkDevice VulkanInfoFromWindow::device() const
{
  return window->device();
}

inline VkPhysicalDevice VulkanInfoFromWindow::physicalDevice() const
{
  return window->physicalDevice();
}

inline VkCommandPool VulkanInfoFromWindow::graphicsCommandPool() const
{
  return window->graphicsCommandPool();
}

inline VkQueue VulkanInfoFromWindow::graphicsQueue() const
{
  return window->graphicsQueue();
}

inline uint32_t VulkanInfoFromWindow::graphicsQueueFamilyIndex() const
{
  return window->graphicsQueueFamilyIndex();
}