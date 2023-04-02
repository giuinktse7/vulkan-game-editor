#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QtGui/QScreen>
#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGTextureProvider>

#include <memory>
#include <string>

#include "core/graphics/vulkan_helpers.h"
#include "core/graphics/vulkan_screen_texture.h"
#include "core/map_renderer.h"
#include "core/map_view.h"
#include "enum_conversion.h"
#include "qt_vulkan_info.h"

class QmlMapItem;
class Position;

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
    bool empty();

    void addTab(std::string tabName);
    void removeTab(int index);
    void removeTabById(int id);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int size();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  signals:
    void sizeChanged(int size);

  protected:
    QHash<int, QByteArray> roleNames() const;

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

class MapTextureNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

  public:
    MapTextureNode(QmlMapItem *item, std::shared_ptr<MapView> mapView, std::shared_ptr<VulkanInfo> &vulkanInfo, uint32_t width, uint32_t height);
    ~MapTextureNode();

    QSGTexture *texture() const override;
    void freeTexture();

    void releaseResources();

    void sync();

  private slots:
    void render();

  private:
    QmlMapItem *m_item = nullptr;
    QQuickWindow *m_window = nullptr;
    QSize textureSize;
    qreal devicePixelRatio;

    std::shared_ptr<VulkanInfo> vulkanInfo;
    std::weak_ptr<MapView> mapView;
    std::unique_ptr<MapRenderer> mapRenderer;

    std::unique_ptr<QSGTexture> textureWrapper;

    VulkanScreenTexture screenTexture;
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

    Q_INVOKABLE void onMousePositionChanged(int x, int y, int button, int buttons, int modifiers);
    Q_INVOKABLE void setFocus(bool focus);

    Q_INVOKABLE void setHorizontalScrollPosition(float value);
    Q_INVOKABLE void setVerticalScrollPosition(float value);

    float horizontalScrollSize();
    float verticalScrollSize();

    float horizontalScrollPosition();
    float verticalScrollPosition();

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
    void releaseResources() override;
    void initialize();

    bool containsMouse() const;

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

    MapTextureNode *textureNode = nullptr;

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
};

inline VME::MouseEvent vmeMouseEvent(QMouseEvent *event)
{
    VME::MouseButtons buttons = enum_conversion::vmeButtons(event->buttons());
    VME::ModifierKeys modifiers = enum_conversion::vmeModifierKeys(event->modifiers());

    ScreenPosition pos(event->pos().x(), event->pos().y());
    return VME::MouseEvent(pos, buttons, modifiers);
}