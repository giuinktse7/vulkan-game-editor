#pragma once

#include <QVulkanWindow>
#include <memory>

#include "../map_renderer.h"
#include "../trianglerenderer.h"
#include "../map_view.h"

class VulkanWindow : public QVulkanWindow
{
public:
  VulkanWindow(MapView *mapView);

  QVulkanWindowRenderer *createRenderer() override;

  MapView *getMapView() const;

protected:
private:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

  // std::unique_ptr<TriangleRenderer> renderer;
  MapRenderer *renderer;
  MapView *mapView;
};
