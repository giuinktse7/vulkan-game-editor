#pragma once

#include <QVulkanWindow>
#include <memory>

#include "map_renderer.h"
#include "trianglerenderer.h"

class MapView;

class VulkanWindow : public QVulkanWindow
{
public:
  VulkanWindow(std::unique_ptr<MapView> mapView);

  QVulkanWindowRenderer *createRenderer() override;

  MapView *getMapView() const;

private:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

  // std::unique_ptr<TriangleRenderer> renderer;
  std::unique_ptr<MapRenderer> renderer;
  std::unique_ptr<MapView> mapView;
};