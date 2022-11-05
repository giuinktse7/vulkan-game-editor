#pragma once
#include <optional>
#include <vulkan/vulkan.h>

#include "vulkan_helpers.h"

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

    // VkPhysicalDevice pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    // VkDevice createLogicalDevice();

    QueueFamilyIndices getQueueFamilies(VulkanInfo &info);
    QueueFamilyIndices findQueueFamilies(VkSurfaceKHR surface, VkPhysicalDevice device);
}; // namespace DeviceManager
