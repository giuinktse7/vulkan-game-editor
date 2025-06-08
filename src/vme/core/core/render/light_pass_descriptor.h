#pragma once

#include "../graphics/buffer.h"
#include "light_occlusion.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

/**
 * @brief Push constant data for light source rendering
 *
 * Optimized structure for passing light data to shaders via push constants.
 * Contains flattened position data for efficient GPU transfer.
 */
struct LightSourcePushConstant
{
    glm::vec4 color;
    glm::vec2 pos;
    float intensity;
    int z;
};

class LightPassDescriptor
{
  public:
    LightPassDescriptor(VulkanInfo *vulkanInfo);
    ~LightPassDescriptor();
    VkDescriptorSetLayout layout;
    VkDescriptorSet descriptorSet;

    struct UniformBufferObject
    {
        glm::mat4 projectionMatrix;
        glm::mat4 inverseProjectionMatrix;
        int floor;
    } ubo;
    BoundBuffer uniformBuffer;
    
    OcclusionData occlusionData;

    void allocate(VkDevice device, VkDescriptorPool pool, VkImageView lightMaskAttachmentView);
    void update(VkDevice device, const VkDescriptorBufferInfo &uniformBufferInfo, const VkDescriptorBufferInfo &occlusionBufferInfo, const VkDescriptorImageInfo &lightMaskInfo);

    /**
     * VkSampler controlling how textures are sampled in the shader.
     *
     * Defines how pixel values (texels) are fetched and interpolated when using
     * texture(sampler, uv) in GLSL.
     */
    VkSampler lightMapSampler = VK_NULL_HANDLE;

  private:
    void createLayout(VkDevice device);
    void createSampler(VkDevice device);
    void createBuffers(VulkanInfo *vulkanInfo);

    VulkanInfo *vulkanInfo = nullptr;
};