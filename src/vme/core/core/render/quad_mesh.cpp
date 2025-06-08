#include "quad_mesh.h"
#include <cstring>

QuadMesh::QuadMesh(VulkanInfo &vulkanInfo)
    : vulkanInfo(vulkanInfo)
{
    createVertexBuffer();
    createIndexBuffer();
}

QuadMesh::~QuadMesh()
{
    indexBuffer.releaseResources();
    vertexBuffer.releaseResources();
}

void QuadMesh::bind(VkCommandBuffer commandBuffer) const
{
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
}

void QuadMesh::createVertexBuffer()
{
    std::array<glm::ivec2, 4> vertices{{{0, 0}, {0, 1}, {1, 1}, {1, 0}}};
    uploadDataToGPU(vertexBuffer, vertices.data(), vertices.size() * sizeof(glm::ivec2), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void QuadMesh::createIndexBuffer()
{
    std::array<uint16_t, 6> indices{0, 1, 3, 3, 1, 2};
    uploadDataToGPU(indexBuffer, indices.data(), indices.size() * sizeof(uint16_t), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

void QuadMesh::uploadDataToGPU(BoundBuffer &targetBuffer, const void *data, VkDeviceSize size, VkBufferUsageFlags usage)
{
    Buffer::CreateInfo stagingInfo;
    stagingInfo.vulkanInfo = &vulkanInfo;
    stagingInfo.size = size;
    stagingInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingInfo.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    BoundBuffer stagingBuffer = Buffer::create(stagingInfo);

    void *mappedData;
    vulkanInfo.vkMapMemory(stagingBuffer.deviceMemory, 0, size, 0, &mappedData);
    memcpy(mappedData, data, size);

    Buffer::CreateInfo bufferInfo;
    bufferInfo.vulkanInfo = &vulkanInfo;
    bufferInfo.size = size;
    bufferInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage;
    bufferInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    targetBuffer = Buffer::create(bufferInfo);

    VkCommandBuffer commandBuffer = vulkanInfo.beginSingleTimeCommands();
    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    vulkanInfo.vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, targetBuffer.buffer, 1, &copyRegion);
    vulkanInfo.endSingleTimeCommands(commandBuffer);

    vulkanInfo.vkUnmapMemory(stagingBuffer.deviceMemory);
}

VkVertexInputBindingDescription QuadMesh::getBindingDescription()
{
    VkVertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(glm::ivec2);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 1> QuadMesh::getAttributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    return attributeDescriptions;
}