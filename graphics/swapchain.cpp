#include "swapchain.h"

#include <algorithm>
#include <stdexcept>

#include "vulkan_helpers.h"
#include "engine.h"
#include "../logger.h"

void SwapChain::initialize()
{
  create();
  createImageViews();
}

void SwapChain::create()
{
  SwapChainSupportDetails swapChainSupport = querySupport(g_engine->getPhysicalDevice(), g_engine->getSurface());

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  // TODO (if this function will be used) fix width & height
  int width = 0;
  int height = 0;
  this->extent = chooseSwapExtent(swapChainSupport.capabilities, width, height);

  minImageCount = swapChainSupport.capabilities.minImageCount;
  imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
  {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = g_engine->getSurface();

  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = g_engine->getQueueFamilyIndices();
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily)
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  //  if (g_vk->vkCreateSwapchainKHR(g_engine->getDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS)
  //  {
  //    throw std::runtime_error("failed to create swap chain!");
  //  }

  // vkGetSwapchainImagesKHR(g_engine->getDevice(), swapChain, &imageCount, nullptr);
  // images.resize(imageCount);
  // vkGetSwapchainImagesKHR(g_engine->getDevice(), swapChain, &imageCount, images.data());

  imageFormat = surfaceFormat.format;
}

void SwapChain::recreate()
{
  // Logger::info("Recreating swap chain");

  g_vk->vkDeviceWaitIdle(g_engine->getDevice());

  if (!g_engine->isValidWindowSize())
  {
    this->validExtent = false;
    return;
  }

  cleanup();

  create();

  createImageViews();

  g_engine->cleanupSyncObjects();
  g_engine->createSyncObjects();
  this->validExtent = true;
}

VkSwapchainKHR &SwapChain::get()
{
  return swapChain;
}

SwapChainSupportDetails SwapChain::querySupport(const VkPhysicalDevice &device, const VkSurfaceKHR &surface)
{
  SwapChainSupportDetails details;

  // g_vk->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount;
  // g_vk->vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0)
  {
    details.formats.resize(formatCount);
    // g_vk->vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  // g_vk->vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

  if (presentModeCount != 0)
  {
    details.presentModes.resize(presentModeCount);
    // g_vk->vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
  for (const auto &availableFormat : availableFormats)
  {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return availableFormat;
    }
  }

  return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
  // for (const auto &availablePresentMode : availablePresentModes)
  // {
  //   if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
  //   {
  //     return availablePresentMode;
  //   }
  // }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, int width, int height)
{
  if (capabilities.currentExtent.width != UINT32_MAX)
  {
    return capabilities.currentExtent;
  }
  else
  {
    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    actualExtent.width = std::max(
        capabilities.minImageExtent.width,
        std::min(capabilities.maxImageExtent.width, actualExtent.width));

    actualExtent.height = std::max(
        capabilities.minImageExtent.height,
        std::min(capabilities.maxImageExtent.height, actualExtent.height));

    return actualExtent;
  }
}

void SwapChain::createImageViews()
{

  imageViews.resize(images.size());

  for (size_t i = 0; i < images.size(); i++)
  {
    imageViews[i] = VulkanHelpers::createImageView(g_engine->getDevice(), images[i], imageFormat);
  }
}

void SwapChain::cleanup()
{
  VkDevice device = g_engine->getDevice();

  for (auto imageView : imageViews)
  {
    g_vk->vkDestroyImageView(device, imageView, nullptr);
  }

  // g_vk->vkDestroySwapchainKHR(device, swapChain, nullptr);
}
