#include "qt_vulkan_info.h"

#include "../gui/vulkan_window.h"

void QtVulkanInfo::update()
{
    df = window->vulkanInstance()->deviceFunctions(device());
    f = window->vulkanInstance()->functions();
}

void QtVulkanInfo::frameReady()
{
    window->frameReady();

    while (!window->waitingForDraw.empty())
    {
        window->waitingForDraw.front()();
        window->waitingForDraw.pop();
    }
}

void QtVulkanInfo::requestUpdate()
{
    window->requestUpdate();
}

int QtVulkanInfo::maxConcurrentFrameCount() const
{
    return window->MAX_CONCURRENT_FRAME_COUNT;
}

util::Size QtVulkanInfo::vulkanSwapChainImageSize() const
{
    QSize size = window->swapChainImageSize();
    return util::Size(size.width(), size.height());
}

VkDevice QtVulkanInfo::device() const
{
    return window->device();
}

VkPhysicalDevice QtVulkanInfo::physicalDevice() const
{
    return window->physicalDevice();
}

VkCommandPool QtVulkanInfo::graphicsCommandPool() const
{
    return window->graphicsCommandPool();
}

VkQueue QtVulkanInfo::graphicsQueue() const
{
    return window->graphicsQueue();
}

uint32_t QtVulkanInfo::graphicsQueueFamilyIndex() const
{
    return window->graphicsQueueFamilyIndex();
}