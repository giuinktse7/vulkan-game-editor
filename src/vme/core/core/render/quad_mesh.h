#pragma once

#include "../graphics/buffer.h"
#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

class QuadMesh
{
  public:
    static constexpr std::array<glm::ivec2, 4> vertices{{{0, 0}, {0, 1}, {1, 1}, {1, 0}}};
    static constexpr std::array<uint32_t, 6> indices{0, 1, 3, 3, 1, 2};

    QuadMesh(VulkanInfo &vulkanInfo);
    ~QuadMesh();

    [[nodiscard]] VkBuffer getVertexBuffer() const;
    [[nodiscard]] VkBuffer getIndexBuffer() const;
    [[nodiscard]] uint32_t getIndexCount() const;

    void bind(VkCommandBuffer commandBuffer) const;

    [[nodiscard]] static VkVertexInputBindingDescription getBindingDescription();
    [[nodiscard]] static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions();

  private:
    VulkanInfo &vulkanInfo;

    BoundBuffer indexBuffer;
    BoundBuffer vertexBuffer;

    void uploadDataToGPU(BoundBuffer &targetBuffer, const void *data, VkDeviceSize size, VkBufferUsageFlags usage);
    void createVertexBuffer();
    void createIndexBuffer();
};
