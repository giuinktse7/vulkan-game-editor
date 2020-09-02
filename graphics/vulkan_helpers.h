#pragma once

#include <vector>
#include <optional>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <QVulkanWindow>
#include <QVulkanFunctions>

#include <glm/glm.hpp>

inline QVulkanDeviceFunctions *g_vk;
inline QVulkanFunctions *g_vkf;
inline QVulkanWindow *g_window;

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

namespace VulkanHelpers
{
  static const char *khronosValidation = "VK_LAYER_KHRONOS_validation";
  static const char *standardValidation = "VK_LAYER_LUNARG_standard_validation";

  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  VkImageView createImageView(VkDevice device, VkImage image, VkFormat format);

  VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);

  uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

  void createCommandPool(VkCommandPool *commandPool, VkCommandPoolCreateFlags flags);
  void createCommandBuffers(VkCommandBuffer *commandBuffer, uint32_t commandBufferCount, VkCommandPool &commandPool);

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer buffer);
  VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t> &code);

  void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

} // namespace VulkanHelpers