#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include <QApplication>
#include <QDataStream>
#include <QDebug>
#include <QMenu>
#include <QMimeData>
#include <QPixmap>
#include <QVulkanWindow>

#include "../graphics/vulkan_helpers.h"
#include "../map_renderer.h"
#include "../map_view.h"
#include "../qt/qt_vulkan_info.h"
#include "../util.h"
#include "draggable_item.h"

class MainWindow;
class Item;

QT_BEGIN_NAMESPACE
class QWidget;
class QPoint;
class QEvent;
QT_END_NAMESPACE

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
    LowerFloorShade,
    Rotate
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

    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);

    void shortcutPressedEvent(ShortcutAction action, QKeyEvent *event = nullptr);
    void shortcutReleasedEvent(ShortcutAction action, QKeyEvent *event = nullptr);

    void onVisibilityChanged(QWindow::Visibility visibility);

    void mapItemDragStartEvent(Tile *tile, Item *item);

    std::optional<ShortcutAction> getShortcutAction(QKeyEvent *event) const;

    void setShortcut(Qt::KeyboardModifiers modifiers, Qt::Key key, ShortcutAction shortcut);
    void setShortcut(Qt::Key key, ShortcutAction shortcut);

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
      * The key is an int from QKeyCombination::toCombined. (combination of Qt::Key and Qt Modifiers (Ctrl, Shift, Alt))
      */
    vme_unordered_map<int, ShortcutAction> shortcuts;
    vme_unordered_map<ShortcutAction, int> shortcutActionToKeyCombination;

    std::unique_ptr<MapView> mapView;

    QVulkanWindowRenderer *renderer = nullptr;
    ContextMenu *contextMenu = nullptr;

    // Holds the current scroll amount. (see wheelEvent)
    int scrollAngleBuffer = 0;

    std::optional<ItemDrag::DragOperation> dragOperation;
};
