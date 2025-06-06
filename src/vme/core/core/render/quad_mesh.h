#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

class QuadMesh
{
  public:
    static constexpr std::array<glm::ivec2, 4> vertices{{{0, 0}, {0, 1}, {1, 1}, {1, 0}}};
    static constexpr std::array<uint32_t, 6> indices{0, 1, 3, 3, 1, 2};

    QuadMesh(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
    ~QuadMesh();

    [[nodiscard]] VkBuffer getVertexBuffer() const;
    [[nodiscard]] VkBuffer getIndexBuffer() const;
    [[nodiscard]] uint32_t getIndexCount() const;

    void bind(VkCommandBuffer commandBuffer) const;

  private:
    VkDevice device;
    VkPhysicalDevice physicalDevice;
    VkCommandPool commandPool;
    VkQueue graphicsQueue;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;

    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    struct Vertex
    {
        glm::vec2 pos;
        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions();
    };
    void createVertexBuffer();
    void createIndexBuffer();
    void createBuffer(const void *data, VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
    void createStagingBuffer(const void *data, VkDeviceSize size, VkBuffer &buffer, VkDeviceMemory &bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};
