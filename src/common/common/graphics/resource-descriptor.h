#pragma once

#include <vulkan/vulkan.hpp>
#include <QVulkanFunctions>

namespace ResourceDescriptor
{
	VkDescriptorSetLayout createLayout(const VkDevice &device);
	VkDescriptorPool createPool();

} // namespace ResourceDescriptor
