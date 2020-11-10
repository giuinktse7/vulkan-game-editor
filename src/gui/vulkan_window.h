#pragma once

#include <functional>
#include <memory>
// #include <optional>
#include <unordered_set>

#include <QApplication>
#include <QMenu>
#include <QVulkanWindow>

#include "../graphics/vulkan_helpers.h"
#include "../map_renderer.h"
#include "../map_view.h"
#include "../qt/qt_vulkan_info.h"
#include "../util.h"

class MainWindow;

QT_BEGIN_NAMESPACE
class QWidget;
class QPoint;
class QEvent;
QT_END_NAMESPACE

};

class VulkanWindow : public QVulkanWindow
{
  Q_OBJECT
public:
  class Renderer : public QVulkanWindowRenderer
  {
  public:
    Renderer(VulkanWindow &window);
    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

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

  std::queue<std::function<void()>> waitingForDraw;

  QtVulkanInfo vulkanInfo;
  QWidget *widget = nullptr;
  EditorAction &editorAction;

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
