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
#include "src/qt_vulkan_info.h"

class MapTextureNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

  public:
    MapTextureNode(QQuickItem *item, QtVulkanInfo *vulkanInfo, MapRenderer *renderer, uint32_t width, uint32_t height);
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

    MapRenderer *mapRenderer = nullptr;
    QtVulkanInfo *vulkanInfo = nullptr;
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

    void keyPressEvent(QKeyEvent *event) override;

  private slots:
    void invalidateSceneGraph();

  private:
    void releaseResources() override;
    void initialize();

    std::unique_ptr<QtVulkanInfo> vulkanInfo;

    bool m_initialized = false;

    MapTextureNode *node = nullptr;
    EditorAction action;
    std::unique_ptr<MapView> mapView;
    std::unique_ptr<MapRenderer> mapRenderer;
};