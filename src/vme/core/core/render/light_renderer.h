#pragma once

#include "../graphics/vulkan_helpers.h"
#include "../map_view.h"
#include "../position.h"
#include "light_constants.h"
#include "light_pass_descriptor.h"
#include "quad_mesh.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>

/**
 * @brief Data structure representing a light source in the game world
 */
struct LightSourceData
{
    Position pos;
    glm::vec4 color;
    float intensity;
};

/**
 * @brief Container for all light sources on a specific Z-level/floor
 * Previously FloorLightData
 */
struct ZLevelLights
{
    /**
    Each index contains the light sources for a specific Z-level.
    The index corresponds to the Z-level, where index 7 is the ground level.
    */
    std::vector<LightSourceData> lights;
};

class LightRenderer
{

    // @note: You only see lights [7, MAX], or lights [0, 6]. So to support the default of floors [0, 15], 9 is enough here.
    // @note z=0 is used for indoor shadows. But since there are no floors above 0, there cannot be indoor shadows on z=0
    // so we are free to use that range in the occlusion buffer for indoor shadows.

  public:
    LightRenderer(std::shared_ptr<VulkanInfo> &vulkanInfo, std::shared_ptr<MapView> mapView);
    ~LightRenderer();

    void prepare(FrameBufferAttachment lightMaskAttachment);

    FrameBufferAttachment lightAttachment;
    FrameBufferAttachment lightMaskAttachment;

    [[nodiscard]] VkRenderPass getRenderPass() const
    {
        return renderPass;
    }

    void render(VkCommandBuffer cb, int frameIndex);
    void updateUniformBuffer();

    void createResources();
    void destroyResources();

    std::shared_ptr<MapView> mapView;

  private:
    void createPipelineLayout();
    void createGraphicsPipeline();
    void createRenderPass();

    [[nodiscard]] VkDevice getDevice() const
    {
        return vulkanInfo->device();
    }

    QuadMesh quadMesh;
    LightPassDescriptor lightDescriptorSet;

    // Store shader modules for cleanup
    std::vector<VkShaderModule> shaderModules;

    std::array<ZLevelLights, TOTAL_FLOORS> floorLightData;

    std::shared_ptr<VulkanInfo> vulkanInfo;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
};