#include "device_manager.h"

#include <vector>
#include <stdexcept>
#include <set>

#include "swapchain.h"

#include "validation.h"
#include "vulkan_helpers.h"
#include "engine.h"

VkPhysicalDevice DeviceManager::pickPhysicalDevice()
{
  uint32_t deviceCount = 0;
  g_vkf->vkEnumeratePhysicalDevices(g_engine->getVkInstance(), &deviceCount, nullptr);

  if (deviceCount == 0)
  {
    throw std::runtime_error("failed to find GPUs with Vulkan support!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  g_vkf->vkEnumeratePhysicalDevices(g_engine->getVkInstance(), &deviceCount, devices.data());

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

  for (const auto &device : devices)
  {
    if (isDeviceSuitable(device))
    {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE)
  {
    throw std::runtime_error("failed to find a suitable GPU!");
  }

  return physicalDevice;
}

bool DeviceManager::isDeviceSuitable(VkPhysicalDevice device)
{
  QueueFamilyIndices indices = getQueueFamilies(device);

  bool extensionsSupported = VulkanHelpers::checkDeviceExtensionSupport(device);

  bool swapChainAdequate = false;
  if (extensionsSupported)
  {
    SwapChainSupportDetails swapChainSupport = SwapChain::querySupport(device, g_engine->getSurface());
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  g_vkf->vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

VkDevice DeviceManager::createLogicalDevice()
{
  VkPhysicalDevice &physicalDevice = g_engine->getPhysicalDevice();
  QueueFamilyIndices indices = getQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo{};

    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();

  createInfo.pEnabledFeatures = &deviceFeatures;

  createInfo.enabledExtensionCount = static_cast<uint32_t>(VulkanHelpers::deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = VulkanHelpers::deviceExtensions.data();

  if (Validation::enableValidationLayers)
  {
    createInfo.enabledLayerCount = static_cast<uint32_t>(VulkanHelpers::validationLayers.size());
    createInfo.ppEnabledLayerNames = VulkanHelpers::validationLayers.data();
  }
  else
  {
    createInfo.enabledLayerCount = 0;
  }

  VkDevice device;

  if (g_vkf->vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create logical device!");
  }

  g_vk->vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, g_engine->getGraphicsQueue());
  g_vk->vkGetDeviceQueue(device, indices.presentFamily.value(), 0, g_engine->getPresentQueue());

  return device;
}

QueueFamilyIndices DeviceManager::getQueueFamilies(VkPhysicalDevice physicalDevice)
{
  uint32_t queueFamilyCount = 0;
  g_vkf->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  g_vkf->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

  QueueFamilyIndices indices;

  int i = 0;
  for (const auto &queueFamily : queueFamilies)
  {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    // g_vkf->vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, g_engine->getSurface(), &presentSupport);

    if (presentSupport)
    {
      indices.presentFamily = i;
    }

    if (indices.isComplete())
    {
      break;
    }

    i++;
  }

  return indices;
}