#include "light_pass_descriptor.h"

#include <array>
#include <vulkan/vulkan.h>

#include "../graphics/vulkan_helpers.h"

LightPassDescriptor::LightPassDescriptor(VulkanInfo *vulkanInfo) : vulkanInfo(vulkanInfo)
{
    createLayout(vulkanInfo->device());
    createSampler(vulkanInfo->device());

    createBuffers(vulkanInfo);
}

LightPassDescriptor::~LightPassDescriptor()
{
    auto *device = vulkanInfo->device();
    vulkanInfo->vkDestroyDescriptorSetLayout(layout, nullptr);
    vulkanInfo->vkDestroySampler(lightMapSampler, nullptr);

    uniformBuffer.releaseResources();
    occlusionData.buffer.releaseResources();
}

void LightPassDescriptor::createLayout(VkDevice device)
{
    // Binding 0: Light uniform buffer
    VkDescriptorSetLayoutBinding lightBufferObject{};
    lightBufferObject.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    lightBufferObject.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    lightBufferObject.binding = 0;
    lightBufferObject.descriptorCount = 1;

    // Binding 1: Light occlusion data buffer
    VkDescriptorSetLayoutBinding occlusionBufferObject{};
    occlusionBufferObject.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    occlusionBufferObject.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    occlusionBufferObject.binding = 1;
    occlusionBufferObject.descriptorCount = 1;

    // Binding 2: Light mask texture target
    VkDescriptorSetLayoutBinding lightMaskTextureBinding{};
    lightMaskTextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    lightMaskTextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    lightMaskTextureBinding.binding = 2;
    lightMaskTextureBinding.descriptorCount = 1;

    std::array<VkDescriptorSetLayoutBinding, 3> setLayoutBindings = {lightBufferObject, occlusionBufferObject, lightMaskTextureBinding};

    VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
    descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorLayoutCI.pBindings = setLayoutBindings.data();
    descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());

    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayoutCI, nullptr, &layout));
}

void LightPassDescriptor::createSampler(VkDevice device)
{

    /* Create a pixel-perfect sampler:
        - No interpolation
        - No mipmapping
        - No anisotropic filtering
        - Clamp out-of-bounds UVs to the texture edge
    */
    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = sampler.addressModeU;
    sampler.addressModeW = sampler.addressModeU;

    sampler.anisotropyEnable = VK_FALSE;
    sampler.unnormalizedCoordinates = VK_FALSE;
    sampler.compareEnable = VK_FALSE;

    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    VK_CHECK_RESULT(vkCreateSampler(device, &sampler, nullptr, &lightMapSampler));
}

void LightPassDescriptor::allocate(VkDevice device, VkDescriptorPool pool, VkImageView lightMaskAttachmentView)
{
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.pSetLayouts = &layout;
    allocInfo.descriptorSetCount = 1;

    VkDescriptorImageInfo texDescriptorLightMask{};
    texDescriptorLightMask.sampler = lightMapSampler;
    texDescriptorLightMask.imageView = lightMaskAttachmentView;
    texDescriptorLightMask.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
    std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
        // Binding 0: Uniform buffer
        vk::tools::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                                      &uniformBuffer.descriptor),
        // Binding 1: Occlusion buffer
        vk::tools::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                      &occlusionData.buffer.descriptor),
        // Binding 2: Light mask
        vk::tools::writeDescriptorSet(descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                      2, &texDescriptorLightMask)};

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0,
                           nullptr);
}

void LightPassDescriptor::createBuffers(VulkanInfo *vulkanInfo)
{
    uniformBuffer = BoundBuffer(vulkanInfo, sizeof(ubo), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    uniformBuffer.initResources(vulkanInfo);

    occlusionData.buffer = BoundBuffer(vulkanInfo, OcclusionData::size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    occlusionData.buffer.initResources(vulkanInfo);
}