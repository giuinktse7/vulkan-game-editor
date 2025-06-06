#include "buffer.h"

#include <stdexcept>

#include "../debug.h"

BoundBuffer::BoundBuffer() = default;

BoundBuffer::BoundBuffer(VulkanInfo *vulkanInfo, VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags)
    : vulkanInfo(vulkanInfo), size(size), usageFlags(usageFlags), propertyFlags(propertyFlags)
{
    descriptor.buffer = buffer;
    descriptor.offset = 0;
    descriptor.range = size;
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

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vulkanInfo->vkCreateBuffer(&bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vulkanInfo->vkGetBufferMemoryRequirements(buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vulkanInfo->findMemoryType(memRequirements.memoryTypeBits, propertyFlags);

    if (vulkanInfo->vkAllocateMemory(&allocInfo, nullptr, &deviceMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vulkanInfo->vkBindBufferMemory(buffer, deviceMemory, 0);
}

void BoundBuffer::releaseResources()
{
    if (vulkanInfo != nullptr)
    {
        if (buffer != VK_NULL_HANDLE)
        {
            vulkanInfo->vkDestroyBuffer(buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        if (deviceMemory != VK_NULL_HANDLE)
        {
            vulkanInfo->vkFreeMemory(deviceMemory, nullptr);
            deviceMemory = VK_NULL_HANDLE;
        }

        vulkanInfo = nullptr;
    }
}

bool BoundBuffer::hasResources() const noexcept
{
    return vulkanInfo != nullptr;
}

BoundBuffer Buffer::create(const Buffer::CreateInfo &createInfo)
{
    BoundBuffer boundBuffer(createInfo.vulkanInfo, createInfo.size, createInfo.usageFlags, createInfo.memoryFlags);
    boundBuffer.initResources(createInfo.vulkanInfo);

    return boundBuffer;
}

void Buffer::copy(VulkanInfo *vulkanInfo, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = vulkanInfo->beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vulkanInfo->vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vulkanInfo->endSingleTimeCommands(commandBuffer);
}

/*
  Copies the data to GPU memory.
*/
void Buffer::copyToMemory(VulkanInfo *vulkanInfo, VkDeviceMemory bufferMemory, const uint8_t *data, VkDeviceSize size)
{
    void *mapped = nullptr;
    // vkMapMemory allows us to access the memory at the VkDeviceMemory.
    vulkanInfo->vkMapMemory(bufferMemory, 0, size, 0, &mapped);

    memcpy(mapped, data, size);

    // After copying the data to the mapped memory, we unmap it again.
    vulkanInfo->vkUnmapMemory(bufferMemory);
}