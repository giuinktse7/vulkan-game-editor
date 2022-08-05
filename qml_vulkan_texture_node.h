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

class CustomTextureNode : public QSGTextureProvider, public QSGSimpleTextureNode
{
    Q_OBJECT

  public:
    CustomTextureNode(QQuickItem *item, QtVulkanInfo *vulkanInfo, MapRenderer *renderer, uint32_t width, uint32_t height);
    // ~CustomTextureNode() override;

    QSGTexture *texture() const override;

    void sync();

  private slots:
    void render();

  private:
    void initialize();

    enum Stage
    {
        VertexStage,
        FragmentStage
    };
    void prepareShader(Stage stage);
    bool buildTexture(const QSize &size);
    void freeTexture();
    bool createRenderPass();

    QQuickItem *m_item;
    QQuickWindow *m_window;
    QSize m_size;
    qreal m_dpr;

    MapRenderer *mapRenderer;
    QtVulkanInfo *vulkanInfo;
    VulkanScreenTexture screenTexture;
};

class CustomTextureItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

  public:
    CustomTextureItem();

  signals:
    void tChanged();

  protected:
    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

  private slots:
    void invalidateSceneGraph();

  private:
    void releaseResources() override;
    void initialize();

    bool m_initialized = false;

    CustomTextureNode *m_node = nullptr;
    std::unique_ptr<QtVulkanInfo> vulkanInfo;
    EditorAction action;
    std::unique_ptr<MapView> mapView;
    std::unique_ptr<MapRenderer> mapRenderer;
};