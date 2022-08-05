#pragma once

/**
Vulkan does not have the concept of a "default framebuffer", hence it requires an
infrastructure that will own the buffers we will render to before we visualize
them on the screen. This infrastructure is known as the swap chain and must be
created explicitly in Vulkan. The swap chain is essentially a queue of images
that are waiting to be presented to the screen. Our application will acquire such
an image to draw to it, and then return it to the queue. How exactly the queue
works and the conditions for presenting an image from the queue depend on how
the swap chain is set up, but the general purpose of the swap chain is to
synchronize the presentation of images with the refresh rate of the screen.
*/

#include <vector>
#include <vulkan/vulkan.h>

#include "device_manager.h"
#include "vulkan_helpers.h"

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class SwapChain
{
  public:
    SwapChain(VkSurfaceKHR surface, VulkanInfo *vulkanInfo);
    bool isDeviceSuitable();

    const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    void create(uint32_t width, uint32_t height);
    void recreate(uint32_t width, uint32_t height);

    VkSwapchainKHR &get();

    void createFramebuffers();

    VkFormat getImageFormat()
    {
        return _imageFormat;
    }

    std::vector<VkImage> &getImages()
    {
        return swapchainImages;
    }

    VkExtent2D getExtent() const
    {
        return extent;
    }

    bool isValid() const
    {
        return validExtent;
    }

    uint32_t getImageCount()
    {
        return imageCount;
    }

    size_t getImageViewCount()
    {
        return imageViews.size();
    }

    uint32_t getMinImageCount()
    {
        return minImageCount;
    }

    VkImageView getImageView(uint32_t index)
    {
        return imageViews[index];
    }

    VkFormat imageFormat()
    {
        return _imageFormat;
    }

    VkDevice device() const
    {
        return _vulkanInfo->device();
    }

    VkPhysicalDevice physicalDevice() const
    {
        return _vulkanInfo->physicalDevice();
    }

  private:
    static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);
    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    SwapChainSupportDetails querySwapChainSupport();

    bool checkDeviceExtensionSupport();

    void createImageViews();

    void cleanup();

    VkSurfaceKHR _surface;
    VulkanInfo *_vulkanInfo = nullptr;

    bool validExtent = true;
    uint32_t minImageCount;
    uint32_t imageCount;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;

    std::vector<VkImageView> imageViews;

    VkFormat _imageFormat;
    VkExtent2D extent;
};