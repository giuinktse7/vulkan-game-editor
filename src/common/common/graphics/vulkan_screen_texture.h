#pragma once

#include <vulkan/vulkan.h>

#include "vulkan_helpers.h"

class VulkanScreenTexture
{
  public:
    VulkanScreenTexture(VulkanInfo *vulkanInfo);
    ~VulkanScreenTexture();

    VkFramebuffer vkFrameBuffer() const
    {
        return m_textureFramebuffer;
    }

    VkImage texture() const
    {
        return m_texture;
    }

    void recreate(VkRenderPass renderPass, uint32_t width, uint32_t height);
    void releaseResources();

  private:
    VkImage m_texture = VK_NULL_HANDLE;
    VkDeviceMemory m_textureMemory = VK_NULL_HANDLE;
    VkFramebuffer m_textureFramebuffer = VK_NULL_HANDLE;
    VkImageView m_textureView = VK_NULL_HANDLE;

    VulkanInfo *vulkanInfo;
};