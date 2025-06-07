#include "vulkan_helpers.h"
#include <fstream>
#include <iostream>
#include <vulkan/vulkan_core.h>

void FrameBufferAttachment::destroy(VulkanInfo &vulkanInfo)
{
    auto *device = vulkanInfo.device();
    if (image != VK_NULL_HANDLE)
    {
        vulkanInfo.vkDestroyImageView(view, nullptr);
        view = VK_NULL_HANDLE;

        vulkanInfo.vkDestroyImage(image, nullptr);
        image = VK_NULL_HANDLE;

        vulkanInfo.vkFreeMemory(mem, nullptr);
        mem = VK_NULL_HANDLE;
    }
}

namespace vk::tools
{
    void createFramebuffer(
        VulkanInfo &vulkanInfo,
        VkRenderPass renderPass,
        uint32_t width,
        uint32_t height,
        std::span<const VkImageView> attachments,
        VkFramebuffer &frameBuffer)
    {
        VkFramebufferCreateInfo fbufCreateInfo = {};
        fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.pNext = nullptr;
        fbufCreateInfo.renderPass = renderPass;
        fbufCreateInfo.pAttachments = attachments.data();
        fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        fbufCreateInfo.width = width;
        fbufCreateInfo.height = height;
        fbufCreateInfo.layers = 1;

        VK_CHECK_RESULT(vulkanInfo.vkCreateFramebuffer(&fbufCreateInfo, nullptr, &frameBuffer));
    }

    VkPipelineShaderStageCreateInfo loadShader(VulkanInfo &vulkanInfo, const std::string &fileName, VkShaderStageFlagBits stage)
    {
        std::ifstream is(fileName, std::ios::binary | std::ios::in | std::ios::ate);

        if (is.is_open())
        {
            size_t size = is.tellg();
            is.seekg(0, std::ios::beg);
            char *shaderCode = new char[size];
            is.read(shaderCode, size);
            is.close();

            assert(size > 0);

            VkPipelineShaderStageCreateInfo shaderStage = {};
            shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStage.stage = stage;

            VkShaderModule shaderModule{};
            VkShaderModuleCreateInfo moduleCreateInfo{};
            moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            moduleCreateInfo.codeSize = size;
            moduleCreateInfo.pCode = (uint32_t *)shaderCode;

            VK_CHECK_RESULT(vulkanInfo.vkCreateShaderModule(&moduleCreateInfo, nullptr, &shaderModule));

            shaderStage.module = shaderModule;

            delete[] shaderCode;

            shaderStage.pName = "main";
            assert(shaderStage.module != VK_NULL_HANDLE);
            return shaderStage;
        }

        std::cerr << "Error: Could not open shader file \"" << fileName << "\""
                  << "\n";
        exit(1);
    }

    VkWriteDescriptorSet writeDescriptorSet(
        VkDescriptorSet dstSet,
        VkDescriptorType type,
        uint32_t binding,
        VkDescriptorBufferInfo *bufferInfo,
        uint32_t descriptorCount)
    {
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pBufferInfo = bufferInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }

    VkWriteDescriptorSet writeDescriptorSet(
        VkDescriptorSet dstSet,
        VkDescriptorType type,
        uint32_t binding,
        VkDescriptorImageInfo *imageInfo,
        uint32_t descriptorCount)
    {
        VkWriteDescriptorSet writeDescriptorSet{};
        writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeDescriptorSet.dstSet = dstSet;
        writeDescriptorSet.descriptorType = type;
        writeDescriptorSet.dstBinding = binding;
        writeDescriptorSet.pImageInfo = imageInfo;
        writeDescriptorSet.descriptorCount = descriptorCount;
        return writeDescriptorSet;
    }

    void createAttachment(VulkanInfo &vulkanInfo, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, FrameBufferAttachment *attachment)
    {
        VkImageAspectFlags aspectMask = 0;

        attachment->format = format;

        if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
        {
            aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        assert(aspectMask > 0);

        VkImageCreateInfo imageInfo{};
        memset(&imageInfo, 0, sizeof(imageInfo));
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;

        VkMemoryAllocateInfo memAllocInfo{};
        memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs{};

        VK_CHECK_RESULT(vulkanInfo.vkCreateImage(&imageInfo, nullptr, &attachment->image));
        vulkanInfo.vkGetImageMemoryRequirements(attachment->image, &memReqs);
        memAllocInfo.allocationSize = memReqs.size;
        memAllocInfo.memoryTypeIndex =
            vk::tools::findMemoryType(vulkanInfo, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK_RESULT(vulkanInfo.vkAllocateMemory(&memAllocInfo, nullptr, &attachment->mem));
        VK_CHECK_RESULT(vulkanInfo.vkBindImageMemory(attachment->image, attachment->mem, 0));

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = format;
        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        imageViewInfo.subresourceRange = {};
        imageViewInfo.subresourceRange.aspectMask = aspectMask;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;
        imageViewInfo.image = attachment->image;
        VK_CHECK_RESULT(vulkanInfo.vkCreateImageView(&imageViewInfo, nullptr, &attachment->view));
    }

    uint32_t findMemoryType(VulkanInfo &vulkanInfo, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(vulkanInfo.physicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

} // namespace vk::tools
