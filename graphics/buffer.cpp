#include "buffer.h"

#include <stdexcept>
#include "vulkan_helpers.h"

BoundBuffer::BoundBuffer()
    : buffer(VK_NULL_HANDLE), deviceMemory(VK_NULL_HANDLE)
{
}
BoundBuffer::BoundBuffer(VkBuffer buffer, VkDeviceMemory deviceMemory)
    : buffer(buffer), deviceMemory(deviceMemory)
{
}

BoundBuffer::BoundBuffer(BoundBuffer &&other) noexcept
    : buffer(other.buffer), deviceMemory(other.deviceMemory)
{
  other.buffer = VK_NULL_HANDLE;
  other.deviceMemory = VK_NULL_HANDLE;
}

BoundBuffer &BoundBuffer::operator=(BoundBuffer &&other) noexcept
{
  buffer = other.buffer;
  deviceMemory = other.deviceMemory;

  other.buffer = VK_NULL_HANDLE;
  other.deviceMemory = VK_NULL_HANDLE;

  return *this;
}

BoundBuffer::~BoundBuffer()
{
  if (buffer != VK_NULL_HANDLE)
  {
    g_vk->vkDestroyBuffer(g_window->device(), buffer, nullptr);
    buffer = VK_NULL_HANDLE;
  }
  if (deviceMemory != VK_NULL_HANDLE)
  {
    g_vk->vkFreeMemory(g_window->device(), deviceMemory, nullptr);
    deviceMemory = VK_NULL_HANDLE;
  }
}

BoundBuffer Buffer::create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
  VkDevice device = g_window->device();

  VkBuffer buffer;
  VkDeviceMemory bufferMemory;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (g_vk->vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  g_vk->vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(g_window->physicalDevice(), memRequirements.memoryTypeBits, properties);

  if (g_vk->vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate buffer memory!");
  }

  g_vk->vkBindBufferMemory(device, buffer, bufferMemory, 0);

  return BoundBuffer(buffer, bufferMemory);
}

void Buffer::copy(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  VkCommandBuffer commandBuffer = VulkanHelpers::beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  g_vk->vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  VulkanHelpers::endSingleTimeCommands(commandBuffer);
}

/*
  Copies the data to GPU memory.
*/
void Buffer::copyToMemory(VkDeviceMemory bufferMemory, uint8_t *data, VkDeviceSize size)
{
  VkDevice device = g_window->device();
  void *mapped = nullptr;
  // vkMapMemory allows us to access the memory at the VkDeviceMemory.
  g_vk->vkMapMemory(device, bufferMemory, 0, size, 0, &mapped);

  memcpy(mapped, data, size);

  // After copying the data to the mapped memory, we unmap it again.
  g_vk->vkUnmapMemory(device, bufferMemory);
}