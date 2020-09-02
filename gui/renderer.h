#pragma once

#include "vulkan_window.h"

class Renderer : public QVulkanWindowRenderer
{
public:
  Renderer(VulkanWindow *window);

  void preInitResources() override;
  void initResources() override;
  void initSwapChainResources() override;
  void releaseSwapChainResources() override;
  void releaseResources() override;

  void startNextFrame() override;
};