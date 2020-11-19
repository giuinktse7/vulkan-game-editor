#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_set>

#include <QApplication>
#include <QDataStream>
#include <QMenu>
#include <QMimeData>
#include <QPixmap>
#include <QVulkanWindow>

#include "../graphics/vulkan_helpers.h"
#include "../map_renderer.h"
#include "../map_view.h"
#include "../qt/qt_vulkan_info.h"
#include "../util.h"

class MainWindow;
class Item;

QT_BEGIN_NAMESPACE
class QWidget;
class QPoint;
class QEvent;
QT_END_NAMESPACE

class ItemDragOperation
{
public:
  /*
  Defines the available MimeData for a drag & drop operation on a map tab.
*/
  class MimeData : public QMimeData
  {
  public:
    struct MapItem
    {
      MapItem();
      MapItem(MapView *mapView, Tile *tile, Item *item);
      Item moveFromMap();

    private:
      MapView *mapView;
      Tile *tile;
      Item *item;
    };
    MimeData(MapView *mapView, Tile *tile, Item *item);

    static const QString mapItemMimeType()
    {
      static const QString mimeType = "vulkan-game-editor-mimetype:map-item";
      return mimeType;
    }

    // void setItem(Tile *tile, Item *item);
    // int getItem() const;

    bool hasFormat(const QString &mimeType) const override;
    QStringList formats() const override;

    // QVariant retrieveData(const QString &mimeType, QVariant::Type type) const override;

    // static QDataStream &operator<<(QDataStream &out, const RawData &rawData);
    // static QDataStream &operator>>(QDataStream &in, RawData &rawData);
    MimeData::MapItem mapItem;
  };

  ItemDragOperation(MapView *mapView, Tile *tile, Item *item, QWindow *parent);

  void start();
  void finish();
  bool isDragging() const;
  bool mouseMoveEvent(QMouseEvent *event);
  bool mouseReleaseEvent(QMouseEvent *event);
  QObject *hoveredObject() const;
  bool hoversParent() const;

private:
  void sendDragEnterEvent(QObject *object, QPoint position, QMouseEvent *event);
  void sendDragLeaveEvent(QObject *object, QPoint position, QMouseEvent *event);
  void sendDragMoveEvent(QObject *object, QPoint position, QMouseEvent *event);
  void sendDragDropEvent(QObject *object, QPoint position, QMouseEvent *event);

  void setHoveredObject(QObject *object);

  QWindow *_parent;
  QObject *_hoveredObject;

  QPixmap pixmap;
  ItemDragOperation::MimeData mimeData;

  bool dragging;
};

Q_DECLARE_METATYPE(ItemDragOperation::MimeData::MapItem);

enum class ShortcutAction
{
  Undo,
  Redo,
  Pan,
  EyeDropper,
  Escape,
  Delete,
  ResetZoom,
  FloorUp,
  FloorDown,
  LowerFloorShade
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

  MainWindow *mainWindow;

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
  void keyReleaseEvent(QKeyEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  void shortcutPressedEvent(ShortcutAction action, QKeyEvent *event = nullptr);
  void shortcutReleasedEvent(ShortcutAction action, QKeyEvent *event = nullptr);

  void onVisibilityChanged(QWindow::Visibility visibility);

  std::optional<ShortcutAction> getShortcutAction(QKeyEvent *event) const;

  void setShortcut(int keyAndModifiers, ShortcutAction shortcut);

  bool containsMouse() const;

  struct MouseState
  {
    Qt::MouseButtons buttons = Qt::MouseButton::NoButton;
  } mouseState;

  /**
 		*	Keeps track of all VulkanWindow instances. This is necessary for QT to
		* validate VulkanWindow pointers in a QVariant.
		*
		* See:
		* QtUtil::associatedVulkanWindow
	*/
  static std::unordered_set<const VulkanWindow *> instances;

  /**
   * The key is a combination of Qt::Key and Qt Modifiers (Ctrl, Shift, Alt)
   */
  vme_unordered_map<int, ShortcutAction> shortcuts;
  vme_unordered_map<ShortcutAction, int> shortcutActionToKeyCombination;

  std::unique_ptr<MapView> mapView;

  QVulkanWindowRenderer *renderer = nullptr;
  ContextMenu *contextMenu = nullptr;

  // Holds the current scroll amount. (see wheelEvent)
  int scrollAngleBuffer = 0;

  std::optional<ItemDragOperation> dragOperation;
};
