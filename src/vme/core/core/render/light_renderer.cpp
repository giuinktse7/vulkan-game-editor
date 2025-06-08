#include "light_renderer.h"
#include "../graphics/vulkan_helpers.h"
#include "quad_mesh.h"
#include <glm/vec3.hpp>
#include <utility>
#include <vulkan/vulkan_core.h>

LightRenderer::LightRenderer(std::shared_ptr<VulkanInfo> &vulkanInfo, std::shared_ptr<MapView> mapView)
    : vulkanInfo(vulkanInfo),
      mapView(std::move(mapView)),
      quadMesh(*vulkanInfo),
      lightDescriptorSet(vulkanInfo.get())
{
}

LightRenderer::~LightRenderer()
{
    destroyResources();
}

void LightRenderer::render(VkCommandBuffer cb, int frameIndex)
{
    const auto &mapViewport = mapView->getViewport();
    uint32_t width = mapViewport.width;
    uint32_t height = mapViewport.height;

    VkViewport viewport{};
    viewport.width = (float)width;
    viewport.height = (float)height;
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(cb, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.extent.width = width;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cb, 0, 1, &scissor);

    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &lightDescriptorSet.descriptorSet, 0, nullptr);
    std::array<VkDeviceSize, 1> offsets = {0};

    vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    quadMesh.bind(cb);

    // Light offset to center the light in the tile
    const auto lightOffset = WorldPosition(MapTileSize / 2, MapTileSize / 2);

    int viewZ = mapView->z();
    int zMax = viewZ > GROUND_FLOOR ? MAX_FLOOR : GROUND_FLOOR;
    for (int z = zMax; z >= viewZ; --z)
    {
        const auto &lights = floorLightData[z].lights;
        if (lights.empty())
        {
            continue;
        }

        for (int i = 0; i < lights.size(); ++i)
        {
            LightSourceData light = lights[i];
            auto worldPos = light.pos.worldPos() + lightOffset;

            LightSourcePushConstant pc{};
            pc.color = light.color;

            pc.pos = glm::vec2(worldPos.x, worldPos.y);
            pc.intensity = light.intensity;
            pc.z = light.pos.z;

            vkCmdPushConstants(cb, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                               sizeof(LightSourcePushConstant), &pc);

            vkCmdDrawIndexed(cb, 6, 1, 0, 0, 0);
        }
    }
}

void LightRenderer::createRenderPass()
{
    auto *device = getDevice();
    const auto &viewport = mapView->getViewport();
    uint32_t width = viewport.width;
    uint32_t height = viewport.height;

    vk::tools::createAttachment(
        *vulkanInfo,
        width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &lightAttachment);

    VkAttachmentDescription attachment{};
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachment.format = lightAttachment.format;

    std::array<VkAttachmentDescription, 1> attachmentDescs = {attachment};

    std::vector<VkAttachmentReference> colorReferences;
    colorReferences.push_back({.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pDepthStencilAttachment = VK_NULL_HANDLE;

    // Use subpass dependencies for attachment layout transitions
    std::array<VkSubpassDependency, 2> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.pAttachments = attachmentDescs.data();
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

    std::array<VkImageView, 1> attachments;
    attachments[0] = lightAttachment.view;
}

void LightRenderer::createResources()
{
    createRenderPass();
    createGraphicsPipeline();
}

void LightRenderer::destroyResources()
{
    auto *device = getDevice();

    for (auto &shaderModule : shaderModules)
    {
        vkDestroyShaderModule(device, shaderModule, nullptr);
    }
    shaderModules.clear();

    vkDestroyPipeline(device, pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

    if (renderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(device, renderPass, nullptr);
        renderPass = VK_NULL_HANDLE;
    }

    lightAttachment.destroy(*vulkanInfo);
}

void LightRenderer::createGraphicsPipeline()
{
    createPipelineLayout();

    auto *device = getDevice();

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
    inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyState.flags = 0;
    inputAssemblyState.primitiveRestartEnable = VK_FALSE;

    VkPipelineVertexInputStateCreateInfo vertexInputState = {};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(glm::ivec2);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SINT;
    attributeDescriptions[0].offset = 0;

    vertexInputState.vertexBindingDescriptionCount = 1;
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputState.pVertexBindingDescriptions = &bindingDescription;
    vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    // rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    auto colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = colorWriteMask;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_MAX;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_MAX;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

    std::array<VkPipelineColorBlendAttachmentState, 1> blendAttachmentStates = {colorBlendAttachment};

    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendState.pAttachments = blendAttachmentStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.flags = 0;

    VkPipelineMultisampleStateCreateInfo multisampleState{};
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleState.flags = 0;

    std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.pDynamicStates = dynamicStateEnables.data();
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    dynamicState.flags = 0;

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

    shaderStages[0] = vk::tools::loadShader(*vulkanInfo, "./shaders/light/spv/light-vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
    shaderStages[1] =
        vk::tools::loadShader(*vulkanInfo, "./shaders/light/spv/light-frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

    // Store shader modules for cleanup
    shaderModules.push_back(shaderStages[0].module);
    shaderModules.push_back(shaderStages[1].module);

    VkGraphicsPipelineCreateInfo pipelineCI{};
    pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCI.layout = pipelineLayout;
    pipelineCI.renderPass = renderPass;
    pipelineCI.flags = 0;
    pipelineCI.basePipelineIndex = -1;
    pipelineCI.basePipelineHandle = VK_NULL_HANDLE;

    pipelineCI.pInputAssemblyState = &inputAssemblyState;
    pipelineCI.pRasterizationState = &rasterizer;
    pipelineCI.pVertexInputState = &vertexInputState;
    pipelineCI.pColorBlendState = &colorBlendState;
    pipelineCI.pMultisampleState = &multisampleState;
    pipelineCI.pDepthStencilState = VK_NULL_HANDLE;
    pipelineCI.pViewportState = &viewportState;
    pipelineCI.pDynamicState = &dynamicState;
    pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCI.pStages = shaderStages.data();

    VK_CHECK_RESULT(vulkanInfo->vkCreateGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline));
}

void LightRenderer::createPipelineLayout()
{
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(LightSourcePushConstant);

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &lightDescriptorSet.layout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

    VK_CHECK_RESULT(vkCreatePipelineLayout(getDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
}
