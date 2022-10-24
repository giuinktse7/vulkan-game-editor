#pragma once

#include <QtGui/QScreen>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGTextureProvider>

#include "common/graphics/vulkan_helpers.h"
#include "common/graphics/vulkan_screen_texture.h"
#include "common/map_renderer.h"
#include "common/map_view.h"
#include "src/enum_conversion.h"
#include "src/qt_vulkan_info.h"

enum class ShortcutAction
{
    Undo,
    Redo,
    Escape,
    Delete,
    ResetZoom,
    FloorUp,
    FloorDown,
    LowerFloorShade,
    Rotate
};

class MapTextureNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

  public:
    MapTextureNode(QQuickItem *item, std::shared_ptr<MapView> mapView, std::shared_ptr<VulkanInfo> &vulkanInfo, std::shared_ptr<MapRenderer> renderer, uint32_t width, uint32_t height);
    ~MapTextureNode();

    QSGTexture *texture() const override;
    void freeTexture();

    void releaseResources();

    void sync();

  private slots:
    void render();

  private:
    QQuickItem *m_item = nullptr;
    QQuickWindow *m_window = nullptr;
    QSize m_size;
    qreal m_dpr;

    std::shared_ptr<VulkanInfo> vulkanInfo;
    std::weak_ptr<MapView> mapView;
    std::weak_ptr<MapRenderer> mapRenderer;

    VulkanScreenTexture screenTexture;

    std::unique_ptr<QSGTexture> textureWrapper;
};

class QmlMapItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

  public:
    QmlMapItem();
    ~QmlMapItem();

  signals:
    void tChanged();

  protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *event) override;

  private slots:
    void invalidateSceneGraph();

  private:
    void releaseResources() override;
    void initialize();

    bool containsMouse() const;

    void mapViewDrawRequested();

    std::optional<ShortcutAction> getShortcutAction(QKeyEvent *event) const;
    void setShortcut(Qt::KeyboardModifiers modifiers, Qt::Key key, ShortcutAction shortcut);
    void setShortcut(Qt::Key key, ShortcutAction shortcut);

    void shortcutPressedEvent(ShortcutAction action, QKeyEvent *event = nullptr);

    std::shared_ptr<VulkanInfo> vulkanInfo;

    bool m_initialized = false;

    MapTextureNode *mapTextureNode = nullptr;
    EditorAction action;
    std::shared_ptr<MapView> mapView;
    std::shared_ptr<MapRenderer> mapRenderer;

    // Holds the current scroll amount. (see wheelEvent)
    int scrollAngleBuffer = 0;

    /**
     * The key is an int from QKeyCombination::toCombined. (combination of Qt::Key and Qt Modifiers (Ctrl, Shift, Alt))
    */
    vme_unordered_map<int, ShortcutAction> shortcuts;
    vme_unordered_map<ShortcutAction, int> shortcutActionToKeyCombination;
};

inline VME::MouseEvent vmeMouseEvent(QMouseEvent *event)
{
    VME::MouseButtons buttons = enum_conversion::vmeButtons(event->buttons());
    VME::ModifierKeys modifiers = enum_conversion::vmeModifierKeys(event->modifiers());

    ScreenPosition pos(event->pos().x(), event->pos().y());
    return VME::MouseEvent(pos, buttons, modifiers);
}