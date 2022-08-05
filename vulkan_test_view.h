#pragma once

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>

#include <QtCore/QRunnable>
#include <QtQuick/QQuickWindow>

#include <QFile>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <QtQml/qqmlregistration.h>

#include "common/editor_action.h"
#include "common/graphics/vulkan_screen_texture.h"
#include "common/map_renderer.h"
#include "common/position.h"
#include "src/qt_vulkan_info.h"

class MockUIUtils : public UIUtils
{
  public:
    MockUIUtils() {}
    ScreenPosition mouseScreenPosInView() override;

    double screenDevicePixelRatio() override;
    double windowDevicePixelRatio() override;

    VME::ModifierKeys modifiers() const override;

    void waitForDraw(std::function<void()> f) override;

  private:
};

class MapViewItemRenderer : public QObject
{
    Q_OBJECT
  public:
    MapViewItemRenderer();
    ~MapViewItemRenderer();

    void setT(qreal t)
    {
        m_t = t;
    }
    void setViewportSize(const QSize &size)
    {
        m_viewportSize = size;
    }

    void setWindow(QQuickWindow *window)
    {
        m_window = window;
        vulkanInfo.setQmlWindow(window);
    }

  public slots:
    void frameStart();
    void mainPassRecordingStart();

  private:
    QSize m_viewportSize;
    qreal m_t = 0;

    QQuickWindow *m_window;

    bool m_initialized = false;

    QtVulkanInfo vulkanInfo;
    MapRenderer renderer;
    MapView mapView;
    EditorAction action;

    // Must be initialized after vulkanInfo & renderer
    VulkanScreenTexture screenTexture;
};

class MapViewItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(qreal t READ t WRITE setT NOTIFY tChanged)
    QML_ELEMENT

  public:
    MapViewItem();

    qreal t() const
    {
        return m_t;
    }
    void setT(qreal t);

  signals:
    void tChanged();

  public slots:
    void sync();
    void cleanup();

  private slots:
    void handleWindowChanged(QQuickWindow *win);

  private:
    void releaseResources() override;

    qreal m_t = 0;
    MapViewItemRenderer *m_renderer = nullptr;
};
