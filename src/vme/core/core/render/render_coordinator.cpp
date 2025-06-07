#include "render_coordinator.h"
#include <cassert>
#include <vector>
#include <vulkan/vulkan_core.h>

RenderCoordinator::RenderCoordinator(std::shared_ptr<VulkanInfo> vulkanInfo,
                                     std::shared_ptr<MapView> mapView,
                                     uint32_t width,
                                     uint32_t height)
    : vulkanInfo(std::move(vulkanInfo)),
      width(width),
      height(height),
      mapRenderer(vulkanInfo, mapView),
      lightRenderer(vulkanInfo, mapView)
{
    // Initialize per-frame resources based on max concurrent frame count
    frameResources.resize(this->vulkanInfo->maxConcurrentFrameCount());

    for (int i = 0; i < frameResources.size(); ++i)
    {
        frameResources[i].index = i; // Set the index based on the position in the vector
    }

    createDescriptorPools();

    // TODO: Very ugly, must be refactored to handle descriptor pools properly
    for (int i = 0; i < frameResources.size(); ++i)
    {
        mapRenderer.setCurrentFrame(i);
        mapRenderer.currentFrame()->descriptorPool = frameResources[i].descriptorPool;
    }
    mapRenderer.setCurrentFrame(0);
    mapRenderer.createResources();
    lightRenderer.createResources();

    // Create framebuffer resources after renderers are initialized
    createFrameBufferResources();
}

void RenderCoordinator::recordFrame(VkCommandBuffer cb, uint32_t frameIndex)
{
    // VME_LOG_D("RenderCoordinator::recordFrame: frameIndex: " << frameIndex);
    // Get the current frame resources
    auto &frameResource = frameResources[frameIndex];

    // Pass 1: Light
    // vkCmdBeginRenderPass(cb, &frameResource.lightRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    // lightRenderer.render(cb, frameIndex);
    // vkCmdEndRenderPass(cb);    // Pass 2: Map
    // TESTING: Only clear color, no draw calls
    vkCmdBeginRenderPass(cb, &frameResource.mapRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    mapRenderer.render(cb, frameIndex);
    vkCmdEndRenderPass(cb);
}

void RenderCoordinator::createFrameBufferResources()
{
    if (hasFrameBufferResources)
    {
        VME_LOG_D("createFrameBufferResources (already created)");
        return;
    }

    VME_LOG_D("createFrameBufferResources");

    // Create resources for each frame
    for (auto &frameResource : frameResources)
    {
        createAttachments(frameResource);
        createMapRendererFrameBuffer(frameResource);
        createLightRendererFrameBuffer(frameResource);
        frameResource.mapRenderPassBeginInfo = mapRenderPassInfo(frameResource.mapFrameBuffer.framebuffer);

        // Set up persistent clear values and update pointers
        constexpr VkClearColorValue MapClearColor = {{0.0F, 0.0F, 0.0F, 1.0F}}; // Black

        frameResource.mapClearValue.color = MapClearColor;
        frameResource.mapRenderPassBeginInfo.pClearValues = &frameResource.mapClearValue;

        frameResource.lightRenderPassBeginInfo = lightRenderPassInfo(frameResource.lightFrameBuffer.framebuffer);
    }

    hasFrameBufferResources = true;
    VME_LOG_D("end of createFrameBufferResources");
}

void RenderCoordinator::createDescriptorPools()
{
    // Create resources for each frame
    for (auto &frameResource : frameResources)
    {
        if (frameResource.index == 0)
        {
            createDescriptorPool(frameResource);
        }
        else
        {
            // Reuse the descriptor pool from the first frame
            frameResource.descriptorPool = frameResources[0].descriptorPool;

            // TODO Use one descriptor pool per frame
        }
    }
}

// Method to resize resources when needed
void RenderCoordinator::resize(uint32_t newWidth, uint32_t newHeight)
{
    VME_LOG_D("RenderCoordinator::resize");

    // Wait for device to be idle before destroying resources
    // vkDeviceWaitIdle(vkDevice());

    destroyFrameBufferResources();

    width = newWidth;
    height = newHeight;

    createFrameBufferResources();

    mapRenderer.setRenderTargetSize(width, height);
}

void RenderCoordinator::createMapRendererFrameBuffer(FrameResources &frameResource)
{
    frameResource.mapFrameBuffer.attachmentViews.clear();

    // Index 0: Map attachment
    frameResource.mapFrameBuffer.attachmentViews.push_back(frameResource.mapAttachment.view);

    // // Index 1: Light mask attachment
    // frameResource.mapFrameBuffer.attachmentViews.push_back(frameResource.lightMaskAttachment.view);

    // // Index 2: Indoor shadow attachment
    // frameResource.mapFrameBuffer.attachmentViews.push_back(frameResource.indoorShadowAttachment.view);

    if (mapRenderer.getRenderPass() == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Map renderer render pass is not initialized.");
    }
    vk::tools::createFramebuffer(
        *vulkanInfo,
        mapRenderer.getRenderPass(), width, height, frameResource.mapFrameBuffer.attachmentViews, frameResource.mapFrameBuffer.framebuffer);
}

void RenderCoordinator::createLightRendererFrameBuffer(FrameResources &frameResource)
{
    frameResource.lightFrameBuffer.attachmentViews.clear();

    // Index 0: Light mask attachment
    frameResource.lightFrameBuffer.attachmentViews.push_back(frameResource.lightMaskAttachment.view);

    if (lightRenderer.getRenderPass() == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Light renderer render pass is not initialized.");
    }
    vk::tools::createFramebuffer(
        *vulkanInfo, lightRenderer.getRenderPass(), width, height, frameResource.lightFrameBuffer.attachmentViews, frameResource.lightFrameBuffer.framebuffer);
}

void RenderCoordinator::createAttachments(FrameResources &frameResource)
{
    auto *device = vulkanInfo->device();
    auto *physicalDevice = vulkanInfo->physicalDevice();

    constexpr VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    constexpr VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    vk::tools::createAttachment(*vulkanInfo, width, height, format, usageFlags, &frameResource.mapAttachment);
    vk::tools::createAttachment(*vulkanInfo, width, height, format, usageFlags, &frameResource.lightMaskAttachment);
    vk::tools::createAttachment(*vulkanInfo, width, height, format, usageFlags, &frameResource.indoorShadowAttachment);
}

void RenderCoordinator::destroyFrameBufferResources()
{
    if (!hasFrameBufferResources)
    {
        VME_LOG_D("destroyFrameBufferResources (no resources to destroy)");
        return;
    }

    VME_LOG_D("destroyFrameBufferResources");
    // Wait for device to be idle before resizing resources
    // vkDeviceWaitIdle(device);

    for (auto &frameResource : frameResources)
    {
        vulkanInfo->vkDestroyFramebuffer(frameResource.mapFrameBuffer.framebuffer, nullptr);
        vulkanInfo->vkDestroyFramebuffer(frameResource.lightFrameBuffer.framebuffer, nullptr);

        frameResource.mapAttachment.destroy(*vulkanInfo);
        frameResource.lightMaskAttachment.destroy(*vulkanInfo);
        frameResource.indoorShadowAttachment.destroy(*vulkanInfo);
    }

    hasFrameBufferResources = false;
}

void RenderCoordinator::destroyResources()
{

    // Destroy resources for each frame
    for (auto &frameResource : frameResources)
    {
        // if (frameResource.descriptorPool != VK_NULL_HANDLE)
        // {
        //     vulkanInfo->vkDestroyDescriptorPool(frameResource.descriptorPool, nullptr);
        //     frameResource.descriptorPool = VK_NULL_HANDLE;
        // }

        destroyFrameBufferResources();
    }

    mapRenderer.destroyResources();
    lightRenderer.destroyResources();
}

VkRenderPassBeginInfo RenderCoordinator::mapRenderPassInfo(VkFramebuffer frameBuffer) const
{
    VME_LOG_D("RenderCoordinator::mapRenderPassInfo" << " width: " << width << ", height: " << height);

    assert(mapRenderer.getRenderPass() != VK_NULL_HANDLE && "Map renderer render pass is not initialized.");

    // To see if clear is working - using red as requested
    constexpr VkClearColorValue ClearColor = {{1.0F, 0.0F, 0.0F, 1.0F}};
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = mapRenderer.getRenderPass();
    renderPassInfo.framebuffer = frameBuffer;

    // Explicitly set render area offset and extent
    renderPassInfo.renderArea.offset = {.x = 0, .y = 0};
    renderPassInfo.renderArea.extent.width = width;
    renderPassInfo.renderArea.extent.height = height;

    renderPassInfo.clearValueCount = 1;
    // Note: pClearValues will be set to point to persistent storage in createFrameBufferResources
    renderPassInfo.pClearValues = nullptr; // Will be updated to point to frameResource.mapClearValue

    return renderPassInfo;
}

VkRenderPassBeginInfo RenderCoordinator::lightRenderPassInfo(VkFramebuffer frameBuffer) const
{
    VkCommandBufferBeginInfo cmdBufInfo{};
    cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    // TODO Don't hard-code
    // Ambience: Lower is darker
    // float ambienceFactor = 0.196f;
    float ambienceFactor = 0.1568F;
    const glm::vec4 AMBIENCE = glm::vec4(glm::vec3(ambienceFactor), 0.0f);

    std::array<VkClearValue, 1> clearValues;
    clearValues[0].color = {{AMBIENCE.r, AMBIENCE.g, AMBIENCE.b, AMBIENCE.a}};

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = lightRenderer.getRenderPass();
    renderPassInfo.framebuffer = frameBuffer;
    renderPassInfo.renderArea.extent.width = width;
    renderPassInfo.renderArea.extent.height = height;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    return renderPassInfo;
}

void RenderCoordinator::createDescriptorPool(FrameResources &frameResource)
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    uint32_t descriptorCount = vulkanInfo->maxConcurrentFrameCount() * 2;

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = descriptorCount;

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = MAX_NUM_TEXTURES;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_NUM_TEXTURES;

    if (vulkanInfo->vkCreateDescriptorPool(&poolInfo, nullptr, &frameResource.descriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

bool RenderCoordinator::containsAnimation() const
{
    return mapRenderer.containsAnimation();
}