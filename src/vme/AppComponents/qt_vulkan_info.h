#pragma once

#include <QVulkanDeviceFunctions>
#include <QVulkanFunctions>
#include <QVulkanInstance>
#include <QVulkanWindow>
#include <QtQuick/QQuickWindow>

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>

#include "core/graphics/vulkan_helpers.h"
#include "core/map_view.h"
#include "core/util.h"

// Matrix that adjusts for OpenGL -> Vulkan clip space differences
static const QMatrix4x4 clipCorrectionMatrix = QMatrix4x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);

class QtVulkanInfo : public VulkanInfo
{
  public:
    QtVulkanInfo(QQuickWindow *qml_window);
    QtVulkanInfo();
    ~QtVulkanInfo();

    void setQmlWindow(QQuickWindow *qml_window);

    void update() override;
    void frameReady() override;
    void requestUpdate() override;

    glm::mat4 projectionMatrix(MapView *mapView, util::Size size) const override
    {
        QMatrix4x4 projection = clipCorrectionMatrix; // adjust for Vulkan-OpenGL clip space differences
        QRectF rect;
        const Camera::Viewport &viewport = mapView->getViewport();
        rect.setX(static_cast<qreal>(viewport.x));
        rect.setY(static_cast<qreal>(viewport.y));
        rect.setWidth(size.width() / viewport.zoom);
        rect.setHeight(size.height() / viewport.zoom);
        projection.ortho(rect);

        glm::mat4 data;
        float *ptr = glm::value_ptr(data);
        projection.transposed().copyDataTo(ptr);

        return data;
    }

    util::Size windowSize() const override;

    int maxConcurrentFrameCount() const override;

    VkDevice device() const override;
    VkPhysicalDevice physicalDevice() const override;
    VkCommandPool graphicsCommandPool() const override;
    VkQueue graphicsQueue() const override;
    uint32_t graphicsQueueFamilyIndex() override;

    inline void vkCmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline) override
    {
        df->vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
    }

    inline VkResult vkCreateImage(const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImage *pImoverride) override
    {
        return df->vkCreateImage(device(), pCreateInfo, pAllocator, pImoverride);
    }
    inline VkResult vkCreateImageView(const VkImageViewCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkImageView *pView) override
    {
        return df->vkCreateImageView(device(), pCreateInfo, pAllocator, pView);
    }

    inline VkResult vkCreateFramebuffer(const VkFramebufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer) override
    {
        return df->vkCreateFramebuffer(device(), pCreateInfo, pAllocator, pFramebuffer);
    }

    inline VkResult vkAllocateCommandBuffers(const VkCommandBufferAllocateInfo *pAllocateInfo, VkCommandBuffer *pCommandBuffers) override
    {
        return df->vkAllocateCommandBuffers(device(), pAllocateInfo, pCommandBuffers);
    }
    inline VkResult vkBeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) override
    {
        return df->vkBeginCommandBuffer(commandBuffer, pBeginInfo);
    }
    inline void vkUpdateDescriptorSets(uint32_t descriptorWriteCount, const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet *pDescriptorCopies) override
    {
        df->vkUpdateDescriptorSets(device(), descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    }
    inline VkResult vkAllocateDescriptorSets(const VkDescriptorSetAllocateInfo *pAllocateInfo, VkDescriptorSet *pDescriptorSets) override
    {
        return df->vkAllocateDescriptorSets(device(), pAllocateInfo, pDescriptorSets);
    }
    inline VkResult vkCreateSampler(const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) override
    {
        return df->vkCreateSampler(device(), pCreateInfo, pAllocator, pSampler);
    }
    inline void vkCmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy *pRegions) override
    {
        df->vkCmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }
    inline VkResult vkBindImageMemory(VkImage image, VkDeviceMemory memory, VkDeviceSize memoryOffset) override
    {
        return df->vkBindImageMemory(device(), image, memory, memoryOffset);
    }
    inline VkResult vkAllocateMemory(const VkMemoryAllocateInfo *pAllocateInfo, const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemoverride) override
    {
        return df->vkAllocateMemory(device(), pAllocateInfo, pAllocator, pMemoverride);
    }
    inline void vkFreeMemory(VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkFreeMemory(device(), memory, pAllocator);
    }

    inline void vkDestroyImage(VkImage image, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyImage(device(), image, pAllocator);
    }
    inline void vkDestroyImageView(VkImageView imageView, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyImageView(device(), imageView, pAllocator);
    }

    VkResult vkCreateSwapchainKHR(const VkSwapchainCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) override
    {
        auto vkCreateSwapchainKHR = reinterpret_cast<PFN_vkCreateSwapchainKHR>(f->vkGetDeviceProcAddr(m_dev, "vkCreateSwapchainKHR"));
        return vkCreateSwapchainKHR(m_dev, pCreateInfo, pAllocator, pSwapchain);
    }

    VkResult vkGetSwapchainImagesKHR(VkSwapchainKHR swapchain, uint32_t *pSwapchainImageCount, VkImage *pSwapchainImages) override
    {
        auto vkGetSwapchainImagesKHR = reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(f->vkGetDeviceProcAddr(m_dev, "vkGetSwapchainImagesKHR"));
        return vkGetSwapchainImagesKHR(m_dev, swapchain, pSwapchainImageCount, pSwapchainImages);
    }

    inline void vkDestroySwapchainKHR(VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) override
    {
        auto vkDestroySwapchainKHR = reinterpret_cast<PFN_vkDestroySwapchainKHR>(f->vkGetDeviceProcAddr(m_dev, "vkDestroySwapchainKHR"));
        vkDestroySwapchainKHR(m_dev, swapchain, pAllocator);
    }

    inline VkResult vkCreateShaderModule(const VkShaderModuleCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule) override
    {
        return df->vkCreateShaderModule(device(), pCreateInfo, pAllocator, pShaderModule);
    }
    inline VkResult vkMapMemory(VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void **ppData) override
    {
        return df->vkMapMemory(device(), memory, offset, size, flags, ppData);
    }
    inline void vkUnmapMemory(VkDeviceMemory memory) override
    {
        df->vkUnmapMemory(device(), memory);
    }
    inline void vkCmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy *pRegions) override
    {
        df->vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
    }
    inline VkResult vkCreateDescriptorPool(const VkDescriptorPoolCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool) override
    {
        return df->vkCreateDescriptorPool(device(), pCreateInfo, pAllocator, pDescriptorPool);
    }
    inline VkResult vkCreateDescriptorSetLayout(const VkDescriptorSetLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDescriptorSetLayout *pSetLayoverride) override
    {
        return df->vkCreateDescriptorSetLayout(device(), pCreateInfo, pAllocator, pSetLayoverride);
    }
    inline void vkDestroyShaderModule(VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyShaderModule(device(), shaderModule, pAllocator);
    }
    inline VkResult vkCreateGraphicsPipelines(VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) override
    {
        return df->vkCreateGraphicsPipelines(device(), pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    }
    inline VkResult vkCreatePipelineLayout(const VkPipelineLayoutCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout) override
    {
        return df->vkCreatePipelineLayout(device(), pCreateInfo, pAllocator, pPipelineLayout);
    }
    inline VkResult vkCreateRenderPass(const VkRenderPassCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) override
    {
        return df->vkCreateRenderPass(device(), pCreateInfo, pAllocator, pRenderPass);
    }
    inline void vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin, VkSubpassContents contents) override
    {
        df->vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
    }
    inline void vkCmdEndRenderPass(VkCommandBuffer commandBuffer) override
    {
        df->vkCmdEndRenderPass(commandBuffer);
    }
    inline VkResult vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) override
    {
        return df->vkQueueSubmit(queue, submitCount, pSubmits, fence);
    }
    inline void vkCmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) override
    {
        df->vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    }

    inline void vkDestroyDescriptorPool(VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyDescriptorPool(device(), descriptorPool, pAllocator);
    }

    inline void vkDestroyPipeline(VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyPipeline(device(), pipeline, pAllocator);
    }

    inline void vkDestroyPipelineLayout(VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyPipelineLayout(device(), pipelineLayout, pAllocator);
    }

    inline void vkDestroyRenderPass(VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyRenderPass(device(), renderPass, pAllocator);
    }

    inline void vkDestroyFramebuffer(VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyFramebuffer(device(), framebuffer, pAllocator);
    }

    inline void vkCmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType) override
    {
        df->vkCmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
    }
    inline void vkCmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers, const VkDeviceSize *pOffsets) override
    {
        df->vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
    }
    inline void vkCmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t descriptorSetCount, const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t *pDynamicOffsets) override
    {
        df->vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
    }
    inline void vkCmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override
    {
        df->vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void vkCmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport *pViewports) override
    {
        df->vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
    }
    void vkCmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D *pScissors) override
    {
        df->vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
    }

    void vkDestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyDescriptorSetLayout(device(), descriptorSetLayout, pAllocator);
    }

    VkResult vkCreateBuffer(const VkBufferCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer) override
    {
        return df->vkCreateBuffer(device(), pCreateInfo, pAllocator, pBuffer);
    }
    void vkGetBufferMemoryRequirements(VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements) override
    {
        df->vkGetBufferMemoryRequirements(device(), buffer, pMemoryRequirements);
    }
    VkResult vkBindBufferMemory(VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) override
    {
        return df->vkBindBufferMemory(device(), buffer, memory, memoryOffset);
    }
    void vkDestroyBuffer(VkBuffer buffer, const VkAllocationCallbacks *pAllocator) override
    {
        df->vkDestroyBuffer(device(), buffer, pAllocator);
    }

    VkResult vkEndCommandBuffer(VkCommandBuffer commandBuffer) override
    {
        return df->vkEndCommandBuffer(commandBuffer);
    }

    void vkFreeCommandBuffers(VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer *pCommandBuffers) override
    {
        df->vkFreeCommandBuffers(device(), commandPool, commandBufferCount, pCommandBuffers);
    }

    void vkCmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size, const void *pValues) override
    {
        df->vkCmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
    }

    VkResult vkQueueWaitIdle(VkQueue queue) override
    {
        return df->vkQueueWaitIdle(queue);
    }

    inline void vkGetImageMemoryRequirements(VkImage image, VkMemoryRequirements *pMemoryRequirements) override
    {
        df->vkGetImageMemoryRequirements(device(), image, pMemoryRequirements);
    }
    inline void vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties) override
    {
        f->vkGetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
    }

    void vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount, VkQueueFamilyProperties *pQueueFamilyProperties) override
    {
        f->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    }

    void setMaxConcurrentFrameCount(int count) override
    {
        _maxConcurrentFrameCount = count;
    }

    inline void vkDeviceWaitIdle() override
    {
        df->vkDeviceWaitIdle(device());
    }

  private:
    /* QML Stuff */
    QQuickWindow *qml_window = nullptr;
    VkPhysicalDevice m_physDev = VK_NULL_HANDLE;
    VkDevice m_dev = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;

    QVulkanDeviceFunctions *df = nullptr;
    QVulkanFunctions *f = nullptr;

    int _maxConcurrentFrameCount;
};
