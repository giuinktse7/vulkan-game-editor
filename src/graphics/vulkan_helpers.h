#pragma once

#include <vector>
#include <optional>
#include <stdexcept>

#include <vulkan/vulkan.h>
#include <QVulkanWindow>
#include <QVulkanFunctions>

#include <glm/glm.hpp>

class VulkanInfo
{
public:
  virtual VkDevice device() const = 0;
  virtual VkPhysicalDevice physicalDevice() const = 0;
  virtual VkCommandPool graphicsCommandPool() const = 0;
  virtual VkQueue graphicsQueue() const = 0;
  virtual uint32_t graphicsQueueFamilyIndex() const = 0;

  virtual ~VulkanInfo() = default;

  QVulkanDeviceFunctions *df = nullptr;
};

inline QVulkanDeviceFunctions *g_vk;
inline QVulkanFunctions *g_vkf;

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

  void createCommandPool(VulkanInfo *info, VkCommandPool *commandPool, VkCommandPoolCreateFlags flags);

  VkCommandBuffer beginSingleTimeCommands(VulkanInfo *info);
  void endSingleTimeCommands(VulkanInfo *info, VkCommandBuffer buffer);
  VkShaderModule createShaderModule(VkDevice device, const std::vector<uint8_t> &code);

  void transitionImageLayout(VulkanInfo *info, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
} // namespace VulkanHelpers