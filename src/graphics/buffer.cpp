#include "buffer.h"

#include <stdexcept>
#include "vulkan_helpers.h"

#include "../debug.h"

BoundBuffer::BoundBuffer()
    : vulkanInfo(nullptr), buffer(VK_NULL_HANDLE), deviceMemory(VK_NULL_HANDLE)
{
}

BoundBuffer::BoundBuffer(VulkanInfo *vulkanInfo, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags)
    : vulkanInfo(vulkanInfo), size(size), usageFlags(usageFlags), propertyFlags(propertyFlags)
{
}

BoundBuffer::BoundBuffer(BoundBuffer &&other) noexcept
    : vulkanInfo(other.vulkanInfo),
      buffer(other.buffer),
      deviceMemory(other.deviceMemory),
      size(other.size),
      usageFlags(other.usageFlags),
      propertyFlags(other.propertyFlags)
{
  other.vulkanInfo = nullptr;

  other.buffer = VK_NULL_HANDLE;
  other.deviceMemory = VK_NULL_HANDLE;

  other.size = 0;
  other.usageFlags = 0;
  other.propertyFlags = 0;
}

BoundBuffer &BoundBuffer::operator=(BoundBuffer &&other) noexcept
{
  buffer = other.buffer;
  deviceMemory = other.deviceMemory;
  vulkanInfo = other.vulkanInfo;
  size = other.size;
  usageFlags = other.usageFlags;
  propertyFlags = other.propertyFlags;

  other.buffer = VK_NULL_HANDLE;
  other.deviceMemory = VK_NULL_HANDLE;
  other.vulkanInfo = nullptr;

  other.size = 0;
  other.usageFlags = 0;
  other.propertyFlags = 0;

  return *this;
}

BoundBuffer::~BoundBuffer()
{
  releaseResources();
}

void BoundBuffer::initResources(VulkanInfo *vulkanInfo)
{
  DEBUG_ASSERT(size != 0 && usageFlags != 0 && propertyFlags != 0, "Invalid BoundBuffer state. Cannot initialize resources.");
  this->vulkanInfo = vulkanInfo;
  VkDevice device = vulkanInfo->device();

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usageFlags;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (devFuncs()->vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  devFuncs()->vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = VulkanHelpers::findMemoryType(vulkanInfo->physicalDevice(), memRequirements.memoryTypeBits, propertyFlags);

  if (devFuncs()->vkAllocateMemory(device, &allocInfo, nullptr, &deviceMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate buffer memory!");
  }

  devFuncs()->vkBindBufferMemory(device, buffer, deviceMemory, 0);
}

void BoundBuffer::releaseResources()
{
  if (vulkanInfo != nullptr)
  {
    if (buffer != VK_NULL_HANDLE)
    {
      devFuncs()->vkDestroyBuffer(vulkanInfo->device(), buffer, nullptr);
      buffer = VK_NULL_HANDLE;
    }
    if (deviceMemory != VK_NULL_HANDLE)
    {
      devFuncs()->vkFreeMemory(vulkanInfo->device(), deviceMemory, nullptr);
      deviceMemory = VK_NULL_HANDLE;
    }

    vulkanInfo = nullptr;
  }
}

bool BoundBuffer::hasResources() const noexcept
{
  return vulkanInfo != nullptr;
}

BoundBuffer Buffer::create(VulkanInfo *vulkanInfo, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
{
  BoundBuffer boundBuffer(vulkanInfo, size, usage, properties);
  boundBuffer.initResources(vulkanInfo);

  return boundBuffer;
}

void Buffer::copy(VulkanInfo *vulkanInfo, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  VkCommandBuffer commandBuffer = VulkanHelpers::beginSingleTimeCommands(vulkanInfo);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vulkanInfo->df->vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  VulkanHelpers::endSingleTimeCommands(vulkanInfo, commandBuffer);
}

/*
  Copies the data to GPU memory.
*/
void Buffer::copyToMemory(VulkanInfo *vulkanInfo, VkDeviceMemory bufferMemory, const uint8_t *data, VkDeviceSize size)
{
  VkDevice device = vulkanInfo->device();
  void *mapped = nullptr;
  // vkMapMemory allows us to access the memory at the VkDeviceMemory.
  vulkanInfo->df->vkMapMemory(device, bufferMemory, 0, size, 0, &mapped);

  memcpy(mapped, data, size);

  // After copying the data to the mapped memory, we unmap it again.
  vulkanInfo->df->vkUnmapMemory(device, bufferMemory);
}