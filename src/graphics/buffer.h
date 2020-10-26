#pragma once

#include <vulkan/vulkan.h>

#include "vulkan_helpers.h"

class VulkanInfo;

struct BoundBuffer
{
	BoundBuffer();
	BoundBuffer(VulkanInfo *vulkanInfo, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags);

	BoundBuffer(const BoundBuffer &other) = delete;
	BoundBuffer &operator=(const BoundBuffer &other) = delete;

	BoundBuffer(BoundBuffer &&other) noexcept;
	BoundBuffer &operator=(BoundBuffer &&other) noexcept;

	~BoundBuffer();

	VulkanInfo *vulkanInfo = nullptr;

	VkBuffer buffer = nullptr;
	VkDeviceMemory deviceMemory = nullptr;

	VkDeviceSize size = 0;
	VkBufferUsageFlags usageFlags = 0;
	VkMemoryPropertyFlags propertyFlags = 0;

	void initResources(VulkanInfo *vulkanInfo);
	void releaseResources();

	bool hasResources() const noexcept;
};

namespace Buffer
{
	struct CreateInfo
	{
		VulkanInfo *vulkanInfo;
		VkDeviceSize size;
		VkBufferUsageFlags usageFlags;
		VkMemoryPropertyFlags memoryFlags;
	};

	BoundBuffer create(const CreateInfo &createInfo);
	void copy(VulkanInfo *vulkanInfo, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void copyToMemory(VulkanInfo *vulkanInfo, VkDeviceMemory bufferMemory, const uint8_t *data, VkDeviceSize size);
} // namespace Buffer