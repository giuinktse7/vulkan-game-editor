#pragma once

#include "../graphics/vulkan_helpers.h"
#include "../map_renderer.h"
#include "light_renderer.h"
#include <cstdint>
#include <memory>
#include <vulkan/vulkan_core.h>

static constexpr int MAX_NUM_TEXTURES = 256 * 256;

struct FrameResources
{
    int index = 0;
    FrameBufferAttachment mapAttachment;
    FrameBufferAttachment lightMaskAttachment;
    FrameBufferAttachment indoorShadowAttachment;
    FrameBufferBundle mapFrameBuffer;
    FrameBufferBundle lightFrameBuffer;

    VkRenderPassBeginInfo mapRenderPassBeginInfo;
    VkRenderPassBeginInfo lightRenderPassBeginInfo;
    

    // TODO Use frame-specific descriptor pools
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};

class RenderCoordinator
{
  public:
    RenderCoordinator(std::shared_ptr<VulkanInfo> vulkanInfo, std::shared_ptr<MapView> mapView, uint32_t width, uint32_t height);

    uint32_t width;
    uint32_t height;

    LightRenderer lightRenderer;
    MapRenderer mapRenderer;

    std::vector<FrameResources> frameResources;

    void destroyResources();

    // Get frame resources for a specific frame index
    FrameResources &getFrameResources(uint32_t frameIndex)
    {
        return frameResources[frameIndex];
    }
    [[nodiscard]] const FrameResources &getFrameResources(uint32_t frameIndex) const
    {
        return frameResources[frameIndex];
    }

    void resize(uint32_t newWidth, uint32_t newHeight);

    // Get the number of frame resources
    [[nodiscard]] size_t getFrameResourcesCount() const
    {
        return frameResources.size();
    }

    void recordFrame(VkCommandBuffer cb, uint32_t frameIndex);

    bool containsAnimation() const;

  private:
    [[nodiscard]] VkDevice vkDevice() const
    {
        return vulkanInfo->device();
    }
    void createDescriptorPool(FrameResources &frameResource);

    void createAttachments(FrameResources &frameResource);

    void createMapRendererFrameBuffer(FrameResources &frameResource);
    void createLightRendererFrameBuffer(FrameResources &frameResource);
    void createFrameBufferResources();
    void createDescriptorPools();
    void destroyFrameBufferResources();

    [[nodiscard]] VkRenderPassBeginInfo mapRenderPassInfo(VkFramebuffer frameBuffer) const;
    [[nodiscard]] VkRenderPassBeginInfo lightRenderPassInfo(VkFramebuffer frameBuffer) const;

    std::shared_ptr<VulkanInfo> vulkanInfo;

    bool hasFrameBufferResources = false;
};