#pragma once
#include <vulkan/vulkan.h>
#include <optional>

struct QueueFamilyIndices
{
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete()
  {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

namespace DeviceManager
{

  VkPhysicalDevice pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  VkDevice createLogicalDevice();

  QueueFamilyIndices getQueueFamilies(VkPhysicalDevice physicalDevice);
}; // namespace DeviceManager
