#pragma once

#include <vulkan/vulkan.h>

struct BoundBuffer
{
	BoundBuffer();
	BoundBuffer(VkBuffer buffer, VkDeviceMemory deviceMemory);
	BoundBuffer(const BoundBuffer &other) = delete;
	BoundBuffer &operator=(const BoundBuffer &other) = delete;

	BoundBuffer(BoundBuffer &&other) noexcept;
	BoundBuffer &operator=(BoundBuffer &&other) noexcept;

	VkBuffer buffer = nullptr;
	VkDeviceMemory deviceMemory = nullptr;

	~BoundBuffer();
};

namespace Buffer
{
	BoundBuffer create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
	void copy(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void copyToMemory(VkDeviceMemory bufferMemory, uint8_t *data, VkDeviceSize size);
} // namespace Buffer