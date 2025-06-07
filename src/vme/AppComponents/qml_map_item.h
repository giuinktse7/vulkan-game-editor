#pragma once

#include <QAbstractListModel>
#include <QEvent>
#include <QString>
#include <QTimer>
#include <QtGui/QScreen>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGTextureProvider>

#include <memory>
#include <qsgtexture.h>
#include <string>

#include "core/graphics/vulkan_helpers.h"
#include "core/map_renderer.h"
#include "core/map_view.h"
#include "core/render/light_renderer.h"
#include "core/render/render_coordinator.h"
#include "core/time_util.h"
#include "enum_conversion.h"
#include "qt_vulkan_info.h"

class QmlMapItem;
class Position;

class MapRenderFinishedEvent : public QEvent
{
  public:
    MapRenderFinishedEvent();
};

struct DeferredEvent
{
    VME::MouseEvent event;
    TimePoint timestamp;
};

class MapTabListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int size READ size NOTIFY sizeChanged)

  private:
    struct TabData
    {
        std::string name;
        uint32_t id;
        QmlMapItem *item;
    };

  public:
    enum class Role
    {
        QmlMapItem = Qt::UserRole + 1,
        EntryId = Qt::UserRole + 2
    };

    void setInstance(int index, QmlMapItem *instance);

    MapTabListModel(QObject *parent = 0);

    TabData &get(int index)
    {
        return _data.at(index);
    }

    TabData *getById(int id);

    void clear();
    [[nodiscard]] bool empty() const;

    void addTab(const std::string &tabName);
    void removeTab(int index);
    void removeTabById(int id);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] int size() const;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  signals:
    void sizeChanged(int size);

  protected:
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

  private:
    std::vector<TabData> _data;

    uint32_t _nextId = 0;
};

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
    BrushEyeDrop,
    Rotate
};

class MapViewTextureNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

  public:
    MapViewTextureNode(QmlMapItem *item, const std::shared_ptr<MapView> &mapView, std::shared_ptr<VulkanInfo> &vulkanInfo, uint32_t width, uint32_t height);
    ~MapViewTextureNode();

    [[nodiscard]] QSGTexture *texture() const override;
    void freeTexture();

    void releaseResources();

    void sync();

  private slots:
    void render();

  private:
    void syncTexture();
    QmlMapItem *m_item = nullptr;
    QQuickWindow *m_window = nullptr;
    QSize textureSize;
    qreal devicePixelRatio;

    std::unique_ptr<RenderCoordinator> renderCoordinator;
    std::shared_ptr<VulkanInfo> vulkanInfo;
    std::weak_ptr<MapView> mapView;

    std::unique_ptr<QSGTexture> textureWrapper;

    // QSGTextures wraps the Vulkan textures, so that we can use them in the scene graph.
    std::array<QSGTexture *, 3> qtSceneGraphTextures;
};

class QmlMapItemStore
{
  public:
    static QmlMapItemStore qmlMapItemStore;

    MapTabListModel *mapTabs()
    {
        return &_mapTabs;
    };

  private:
    MapTabListModel _mapTabs;
};

class QmlMapItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString name READ qStrName)
    Q_PROPERTY(int entryId READ entryId WRITE setEntryId NOTIFY entryIdChanged)
    Q_PROPERTY(float horizontalScrollSize READ horizontalScrollSize NOTIFY horizontalScrollSizeChanged)
    Q_PROPERTY(float horizontalScrollPosition READ horizontalScrollPosition NOTIFY horizontalScrollPositionChanged)
    Q_PROPERTY(float verticalScrollSize READ verticalScrollSize NOTIFY verticalScrollSizeChanged)
    Q_PROPERTY(float verticalScrollPosition READ verticalScrollPosition NOTIFY verticalScrollPositionChanged)

  public:
    QmlMapItem(std::string name = "");
    ~QmlMapItem();

    void scheduleDraw(int millis);

    void beforeRenderMap();
    void afterRenderMap();

    Q_INVOKABLE void onMousePositionChanged(int x, int y, int button, int buttons, int modifiers);
    Q_INVOKABLE void setFocus(bool focus);

    Q_INVOKABLE void setHorizontalScrollPosition(float value);
    Q_INVOKABLE void setVerticalScrollPosition(float value);

    [[nodiscard]] float horizontalScrollSize() const;
    [[nodiscard]] float verticalScrollSize() const;

    [[nodiscard]] float horizontalScrollPosition() const;
    [[nodiscard]] float verticalScrollPosition() const;

    void setActive(bool active);

    bool isActive()
    {
        return _active;
    }

    bool hasFocus()
    {
        return _focused;
    }

    void setMap(std::shared_ptr<Map> &&map);

    std::shared_ptr<MapView> mapView;

    int entryId()
    {
        return _id;
    }

    void setEntryId(int id);

  signals:
    void entryIdChanged();
    void handleWindowChanged(QQuickWindow *win);
    void horizontalScrollSizeChanged();
    void horizontalScrollPositionChanged();
    void verticalScrollSizeChanged();
    void verticalScrollPositionChanged();

  protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    bool event(QEvent *event) override;

  public slots:
    void sync();
    void cleanup();

  private slots:
    void invalidateSceneGraph();

  private:
    void delayedUpdate();
    void releaseResources() override;
    void initialize();

    bool containsMouse() const;

    void defer(QEvent::Type eventType, VME::MouseEvent event);

    void mapViewDrawRequested();

    std::optional<ShortcutAction> getShortcutAction(QKeyEvent *event) const;
    void setShortcut(Qt::KeyboardModifiers modifiers, Qt::Key key, ShortcutAction shortcut);
    void setShortcut(Qt::Key key, ShortcutAction shortcut);

    void shortcutPressedEvent(ShortcutAction action, QKeyEvent *event = nullptr);

    QString qStrName();

    void eyedrop(const Position position) const;

    void onMapViewportChanged(const Camera::Viewport &);

    std::string _name;

    std::shared_ptr<VulkanInfo> vulkanInfo;

    MapViewTextureNode *mapViewTextureNode = nullptr;

    // Holds the current scroll amount. (see wheelEvent)
    int scrollAngleBuffer = 0;

    /**
     * The key is an int from QKeyCombination::toCombined. (combination of Qt::Key and Qt Modifiers (Ctrl, Shift, Alt))
     */
    vme_unordered_map<int, ShortcutAction> shortcuts;
    vme_unordered_map<ShortcutAction, int> shortcutActionToKeyCombination;

    int _id = -1;

    bool _focused = false;
    bool _active = false;

    uint32_t minTimePerFrameMs = 1000 / 120; // 120 FPS
    TimePoint lastDrawTime;
    QTimer drawTimer;

    vme_unordered_map<QEvent::Type, DeferredEvent> deferredEvents;

    bool rendering = false;
};

inline VME::MouseEvent vmeMouseEvent(QMouseEvent *event)
{
    VME::MouseButtons buttons = enum_conversion::vmeButtons(event->buttons());
    VME::ModifierKeys modifiers = enum_conversion::vmeModifierKeys(event->modifiers());

    ScreenPosition pos(event->pos().x(), event->pos().y());
    return VME::MouseEvent(pos, buttons, modifiers);
}