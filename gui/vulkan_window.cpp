#include "vulkan_window.h"

#include "map_renderer.h"
#include "map_view.h"

VulkanWindow::VulkanWindow(MapView *mapView)
    : mapView(mapView)
{
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
  // Memory deleted by QT when QT closes
  renderer = new MapRenderer(*this);
  return renderer;
}

void VulkanWindow::mousePressEvent(QMouseEvent *e)
{
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *)
{
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *e)
{
}

void VulkanWindow::keyPressEvent(QKeyEvent *e)
{
}

MapView *VulkanWindow::getMapView() const
{
  return mapView;
}