#include "vulkan_window.h"

#include "map_renderer.h"
#include "map_view.h"

VulkanWindow::VulkanWindow(std::unique_ptr<MapView> mapView)
    : mapView(std::move(mapView))
{
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
  // std::cout << "Creating MapRenderer" << std::endl;
  renderer = std::make_unique<MapRenderer>(*this);
  return renderer.get();
  // renderer = std::make_unique<TriangleRenderer>(this, true);
  // return renderer.get();
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
  return mapView.get();
}