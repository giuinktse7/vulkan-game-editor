#include "vulkan_screen_texture.h"
#include "../log.h"

VulkanScreenTexture::VulkanScreenTexture(VulkanInfo *vulkanInfo)
    : vulkanInfo(vulkanInfo)
{
    VME_LOG_D("VulkanScreenTexture()");
}

VulkanScreenTexture::~VulkanScreenTexture()
{
    VME_LOG_D("~VulkanScreenTexture()");
    releaseResources();
}

void VulkanScreenTexture::releaseResources()
{
    VME_LOG_D("VulkanScreenTexture::releaseResources");
    if (m_texture)
    {
        vulkanInfo->vkDestroyFramebuffer(m_textureFramebuffer, nullptr);
        m_textureFramebuffer = VK_NULL_HANDLE;

        vulkanInfo->vkFreeMemory(m_textureMemory, nullptr);
        m_textureMemory = VK_NULL_HANDLE;

        vulkanInfo->vkDestroyImageView(m_textureView, nullptr);
        m_textureView = VK_NULL_HANDLE;

        vulkanInfo->vkDestroyImage(m_texture, nullptr);
        m_texture = VK_NULL_HANDLE;
    }
}

void VulkanScreenTexture::recreate(VkRenderPass renderPass, uint32_t width, uint32_t height)
{
    VME_LOG_D("VulkanScreenTexture::recreate");
    releaseResources();

    VkImageCreateInfo imageInfo;
    memset(&imageInfo, 0, sizeof(imageInfo));
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = 0;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImage image = VK_NULL_HANDLE;
    if (vulkanInfo->vkCreateImage(&imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        ABORT_PROGRAM("[VulkanScreenTexture] failed to create image!");
    }

    m_texture = image;

    VkMemoryRequirements memRequirements;
    vulkanInfo->vkGetImageMemoryRequirements(m_texture, &memRequirements);

    uint32_t memIndex = 0;
    VkPhysicalDeviceMemoryProperties physDevMemProps;
    vulkanInfo->vkGetPhysicalDeviceMemoryProperties(vulkanInfo->physicalDevice(), &physDevMemProps);
    for (uint32_t i = 0; i < physDevMemProps.memoryTypeCount; ++i)
    {
        if (!(memRequirements.memoryTypeBits & (1 << i)))
        {
            continue;
        }

        memIndex = i;
    }

    VkMemoryAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        nullptr,
        memRequirements.size,
        memIndex};

    VkResult result = vulkanInfo->vkAllocateMemory(&allocInfo, nullptr, &m_textureMemory);
    if (result != VK_SUCCESS)
    {
        ABORT_PROGRAM("[VulkanScreenTexture] Failed to allocate memory for linear image.");
    }

    result = vulkanInfo->vkBindImageMemory(image, m_textureMemory, 0);
    if (result != VK_SUCCESS)
    {
        ABORT_PROGRAM("[VulkanScreenTexture] Failed to bind linear image memory");
    }

    VkImageViewCreateInfo viewInfo;
    memset(&viewInfo, 0, sizeof(viewInfo));
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageInfo.format;
    viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
    viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
    viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
    viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    result = vulkanInfo->vkCreateImageView(&viewInfo, nullptr, &m_textureView);
    if (result != VK_SUCCESS)
    {
        ABORT_PROGRAM("[VulkanScreenTexture] Failed to create render target image view");
    }

    VkFramebufferCreateInfo fbInfo;
    memset(&fbInfo, 0, sizeof(fbInfo));
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = renderPass;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &m_textureView;
    fbInfo.width = width;
    fbInfo.height = height;
    fbInfo.layers = 1;

    result = vulkanInfo->vkCreateFramebuffer(&fbInfo, nullptr, &m_textureFramebuffer);
    if (result != VK_SUCCESS)
    {
        ABORT_PROGRAM("[VulkanScreenTexture] Failed to create framebuffer");
    }
}
