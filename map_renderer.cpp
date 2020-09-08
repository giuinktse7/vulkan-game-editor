#include "map_renderer.h"

#include <stdexcept>

#include "file.h"
#include "position.h"

#include "graphics/resource-descriptor.h"
#include "graphics/appearances.h"
#include "graphics/vulkan_helpers.h"

#include "ecs/ecs.h"
#include "debug.h"
#include "util.h"
#include "logger.h"
#include "gui/vulkan_window.h"

constexpr int GROUND_FLOOR = 7;
constexpr VkClearColorValue ClearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};

namespace colors
{
  constexpr glm::vec4 Default{1.0f, 1.0f, 1.0f, 1.0f};
  constexpr glm::vec4 Selected{0.45f, 0.45f, 0.45f, 1.0f};
  constexpr glm::vec4 SeeThrough{1.0f, 1.0f, 1.0f, 0.35f};
  constexpr glm::vec4 ItemPreview{0.6f, 0.6f, 0.6f, 0.7f};

} // namespace colors

MapRenderer::MapRenderer(VulkanWindow &window)
    : window(window), colorFormat(window.colorFormat())
{
  this->mapView = window.getMapView();
}

void MapRenderer::initResources()
{
  std::cout << "MapRenderer::initResources" << std::endl;
  std::cout << window.device() << std::endl;

  g_vk = window.vulkanInstance()->deviceFunctions(window.device());
  g_vkf = window.vulkanInstance()->functions();
  g_window = &window;
  colorFormat = window.colorFormat();

  createRenderPass();

  currentFrame = &frames.front();

  createDescriptorSetLayouts();
  createGraphicsPipeline();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();

  uint32_t indexSize = 6 * sizeof(uint16_t);

  BoundBuffer indexStagingBuffer = Buffer::create(
      indexSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  void *data;
  g_vk->vkMapMemory(window.device(), indexStagingBuffer.deviceMemory, 0, indexSize, 0, &data);
  uint16_t *indices = reinterpret_cast<uint16_t *>(data);
  std::array<uint16_t, 6> indexArray{0, 1, 3, 3, 1, 2};

  memcpy(indices, &indexArray, sizeof(indexArray));

  indexBuffer = Buffer::create(
      indexSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VkCommandBuffer commandBuffer = VulkanHelpers::beginSingleTimeCommands();
  VkBufferCopy copyRegion = {};
  copyRegion.size = indexSize;
  g_vk->vkCmdCopyBuffer(commandBuffer, indexStagingBuffer.buffer, indexBuffer.buffer, 1, &copyRegion);
  VulkanHelpers::endSingleTimeCommands(commandBuffer);

  g_vk->vkUnmapMemory(window.device(), indexStagingBuffer.deviceMemory);

  std::cout << "End MapRenderer::initResources" << std::endl;
}

void MapRenderer::initSwapChainResources()
{
  const util::Size sz = window.vulkanSwapChainImageSize();
  mapView->setViewportSize(sz.width(), sz.height());
}

void MapRenderer::releaseSwapChainResources()
{
  mapView->setViewportSize(0, 0);
}

void MapRenderer::releaseResources()
{
  std::cout << "MapRenderer::releaseResources" << std::endl;
  debug = true;
  g_vk->vkDestroyDescriptorSetLayout(window.device(), uboDescriptorSetLayout, nullptr);
  g_vk->vkDestroyDescriptorSetLayout(window.device(), textureDescriptorSetLayout, nullptr);

  g_vk->vkDestroyDescriptorPool(window.device(), descriptorPool, nullptr);

  g_vk->vkDestroyPipeline(window.device(), graphicsPipeline, nullptr);
  g_vk->vkDestroyPipelineLayout(window.device(), pipelineLayout, nullptr);
  g_vk->vkDestroyRenderPass(window.device(), renderPass, nullptr);
  this->renderPass = VK_NULL_HANDLE;
  this->indexBuffer = {};

  Texture::Descriptor descriptor;
  descriptor.layout = textureDescriptorSetLayout;
  descriptor.pool = descriptorPool;

  for (Texture *texture : activeTextures)
  {
    texture->releaseVulkanResources();
  }

  Texture::releaseSolidColorTextures();

  for (auto &frame : frames)
  {
    frame.uniformBuffer = {};
    frame.batchDraw = BatchDraw();
  }
}

void MapRenderer::startNextFrame()
{
  // std::cout << "MapRenderer::startNextFrame()" << std::endl;

  // std::cout << "window.currentFrame(): " << window.currentFrame() << std::endl;
  this->currentFrame = &frames[window.currentFrame()];
  currentFrame->commandBuffer = g_window->currentCommandBuffer();
  currentFrame->frameBuffer = g_window->currentFramebuffer();
  currentFrame->batchDraw.commandBuffer = currentFrame->commandBuffer;

  mapView->updateViewport();

  updateUniformBuffer();

  drawMap();

  currentFrame->batchDraw.prepareDraw();

  beginRenderPass();
  g_vk->vkCmdBindPipeline(currentFrame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  drawBatches();
  g_vk->vkCmdEndRenderPass(currentFrame->commandBuffer);

  window.frameReady();
  window.requestUpdate();
}

void MapRenderer::drawBatches()
{
  if (this->debug)
  {
  }
  const util::Size size = window.vulkanSwapChainImageSize();

  VkViewport viewport;
  viewport.x = viewport.y = 0;
  viewport.width = size.width();
  viewport.height = size.height();
  viewport.minDepth = 0;
  viewport.maxDepth = 1;
  g_vk->vkCmdSetViewport(currentFrame->commandBuffer, 0, 1, &viewport);

  VkRect2D scissor;
  scissor.offset.x = scissor.offset.y = 0;
  scissor.extent.width = viewport.width;
  scissor.extent.height = viewport.height;
  g_vk->vkCmdSetScissor(currentFrame->commandBuffer, 0, 1, &scissor);

  VkDeviceSize offsets[] = {0};
  VkBuffer buffers[] = {nullptr};

  VkDescriptorSet currentDescriptorSet = currentFrame->uboDescriptorSet;

  std::array<VkDescriptorSet, 2> descriptorSets = {
      currentDescriptorSet,
      nullptr};

  g_vk->vkCmdBindIndexBuffer(currentFrame->commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

  for (auto &batch : currentFrame->batchDraw.getBatches())
  {
    if (!batch.isValid())
      break;

    buffers[0] = batch.buffer.buffer;
    g_vk->vkCmdBindVertexBuffers(currentFrame->commandBuffer, 0, 1, buffers, offsets);

    uint32_t offset = 0;
    for (const auto &descriptorInfo : batch.descriptorIndices)
    {
      descriptorSets[1] = descriptorInfo.descriptor;

      g_vk->vkCmdBindDescriptorSets(currentFrame->commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    pipelineLayout,
                                    0,
                                    static_cast<uint32_t>(descriptorSets.size()),
                                    descriptorSets.data(),
                                    0,
                                    nullptr);

      // 4 is vertices for one sprite.
      uint32_t sprites = (descriptorInfo.end - offset + 1) / 4;
      for (uint32_t spriteIndex = 0; spriteIndex < sprites; ++spriteIndex)
      {
        g_vk->vkCmdDrawIndexed(currentFrame->commandBuffer, 6, 1, 0, offset + spriteIndex * 4, 0);
      }

      offset = descriptorInfo.end + 1;
    }

    batch.invalidate();
  }
}

void MapRenderer::drawMap()
{
  bool isSelectionMoving = mapView->selection.moving;

  // std::cout << std::endl << "drawMap()" << std::endl;
  const auto mapRect = mapView->getGameBoundingRect();
  int floor = mapView->getZ();

  bool aboveGround = floor <= 7;

  int startZ = aboveGround ? GROUND_FLOOR : MAP_LAYERS - 1;
  int endZ = floor;

  Position from{mapRect.x1, mapRect.y1, startZ};
  Position to{mapRect.x2, mapRect.y2, endZ};
  for (auto &tileLocation : mapView->getMap()->getRegion(from, to))
  {
    if (!tileLocation.hasTile())
    {
      continue;
    }
    /* Avoid drawing the tile only if the whole tile is selected
                 and the selection is moving
              */
    if (!(tileLocation.getTile()->allSelected() && isSelectionMoving))
    {
      drawTile(tileLocation, ItemDrawFlags::DrawSelected);
    }
  }

  // TODO
  bool hasBrush = false;
  // bool hasBrush = g_engine->hasBrush();

  if (hasBrush)
  {
    drawPreviewCursor();
  }

  if (mapView->isSelectionMoving())
  {
    drawMovingSelection();
  }

  if (mapView->isDragging())
  {
    drawSelectionRectangle();
  }

  // drawTestRectangle();
}

void MapRenderer::drawTestRectangle()
{
  Texture::Descriptor descriptor;
  descriptor.layout = textureDescriptorSetLayout;
  descriptor.pool = descriptorPool;

  RectangleDrawInfo info;

  info.from = MapPosition(5, 5).worldPos();
  info.to = MapPosition(12, 12).worldPos();
  Texture *texture = &Texture::getOrCreateSolidTexture(SolidColor::Blue);
  texture->updateVkResources(descriptor);
  info.texture = texture;
  info.color = colors::SeeThrough;

  currentFrame->batchDraw.addRectangle(info);
}
void MapRenderer::drawTile(const TileLocation &tileLocation, uint32_t drawFlags)
{
  auto position = tileLocation.getPosition();
  auto tile = tileLocation.getTile();

  bool drawSelected = drawFlags & ItemDrawFlags::DrawSelected;

  Position selectionMovePosDelta{};
  if (mapView->selection.moving)
  {
    // TODO
    // ScreenPosition cursorPos = g_engine->getCursorPos()
    ScreenPosition cursorPos = ScreenPosition{};
    selectionMovePosDelta = cursorPos.toPos(*mapView) - mapView->selection.moveOrigin.value();
  }

  if (tile->getGround())
  {
    Item *ground = tile->getGround();
    if (drawSelected || !ground->selected)
    {
      ObjectDrawInfo info;
      info.appearance = ground->itemType->appearance;
      info.position = position;
      if (ground->selected)
        info.position += selectionMovePosDelta;

      info.color = ground->selected ? colors::Selected : colors::Default;
      info.textureInfo = ground->getTextureInfo(info.position);

      drawItem(info);
    }
  }

  DrawOffset drawOffset{0, 0};
  auto &items = tile->getItems();

  for (const Item &item : items)
  {
    if (item.selected && !drawSelected)
    {
      continue;
    }

    ObjectDrawInfo info;
    info.appearance = item.itemType->appearance;
    info.color = item.selected ? colors::Selected : colors::Default;
    info.drawOffset = drawOffset;
    info.position = position;
    if (item.selected)
      info.position += selectionMovePosDelta;
    info.textureInfo = item.getTextureInfo(position);

    drawItem(info);

    if (item.itemType->hasElevation())
    {
      uint32_t elevation = item.itemType->getElevation();
      drawOffset.x -= elevation;
      drawOffset.y -= elevation;
    }
  }
}

void MapRenderer::drawPreviewCursor()
{
  // TODO
  uint16_t selectedServerId = 100;
  // uint16_t selectedServerId = g_engine->getSelectedServerId().value();
  ItemType &selectedItemType = *Items::items.getItemType(selectedServerId);

  // TODO
  // ScreenPosition cursorPos = g_engine->getCursorPos()
  ScreenPosition cursorPos = ScreenPosition{};
  Position pos = cursorPos.worldPos(*mapView).mapPos().floor(mapView->getFloor());

  Tile *tile = mapView->getMap()->getTile(pos);

  int elevation = tile ? tile->getTopElevation() : 0;

  ObjectDrawInfo drawInfo;
  drawInfo.appearance = selectedItemType.appearance;
  drawInfo.color = colors::ItemPreview;
  drawInfo.drawOffset = {-elevation, -elevation};
  drawInfo.position = pos;
  drawInfo.textureInfo = selectedItemType.getTextureInfo(pos);

  drawItem(drawInfo);
}

void MapRenderer::drawMovingSelection()
{
  auto mapRect = mapView->getGameBoundingRect();

  Position moveOrigin = mapView->selection.moveOrigin.value();

  // TODO
  // ScreenPosition screenCursorPos = g_engine->getCursorPos()
  ScreenPosition screenCursorPos = ScreenPosition{};
  Position cursorPos = screenCursorPos.toPos(*mapView);

  Position deltaPos = cursorPos - moveOrigin;

  mapRect.x1 -= deltaPos.x;
  mapRect.x1 = std::max(0, mapRect.x1);
  mapRect.x2 -= deltaPos.x;

  mapRect.y1 -= deltaPos.y;
  mapRect.y1 = std::max(0, mapRect.y1);

  mapRect.y2 -= deltaPos.y;

  // TODO: Use selection Z bounds instead of all floors
  int startZ = MAP_LAYERS - 1;
  int endZ = 0;

  Position from(mapRect.x1, mapRect.y1, startZ);
  Position to(mapRect.x2, mapRect.y2, endZ);

  for (auto &tileLocation : mapView->getMap()->getRegion(from, to))
  {
    if (tileLocation.hasTile())
    {
      // Draw only if the tile has a selection.
      if ((tileLocation.getTile()->hasSelection() && mapView->selection.moving))
      {
        drawTile(tileLocation, ItemDrawFlags::DrawSelected);
      }
    }
  }
}

void MapRenderer::drawSelectionRectangle()
{
  RectangleDrawInfo info;

  const auto [from, to] = mapView->getDragPoints().value();
  info.from = from;
  info.to = to;
  // info.texture = Items::items.getItemType(2554)->getTextureInfo();
  info.texture = Texture::getSolidTexture(SolidColor::Blue);
  info.color = colors::SeeThrough;

  currentFrame->batchDraw.addRectangle(info);
}

void MapRenderer::updateUniformBuffer()
{
  ScreenPosition cursorPos(0.0, 0.0);
  mapView->updateCamera();
  glm::mat4 projection = window.projectionMatrix();
  ItemUniformBufferObject uniformBufferObject{projection};

  void *data;
  g_vk->vkMapMemory(window.device(), currentFrame->uniformBuffer.deviceMemory, 0, sizeof(ItemUniformBufferObject), 0, &data);
  memcpy(data, &uniformBufferObject, sizeof(ItemUniformBufferObject));
  g_vk->vkUnmapMemory(window.device(), currentFrame->uniformBuffer.deviceMemory);
}

void MapRenderer::drawItem(ObjectDrawInfo &info)
{
  Texture::Descriptor descriptor;
  descriptor.layout = textureDescriptorSetLayout;
  descriptor.pool = descriptorPool;

  TextureAtlas *atlas = info.textureInfo.atlas;

  if (atlas->isCompressed())
  {
    atlas->decompressTexture();
    activeTextures.emplace_back(atlas->getTexture());
  }

  atlas->getTexture()->updateVkResources(descriptor);

  currentFrame->batchDraw.addItem(info);
}

/**
 * 
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * Start of Vulkan rendering setup/teardown
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 **/
void MapRenderer::beginRenderPass()
{
  util::Size size = window.vulkanSwapChainImageSize();
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = currentFrame->frameBuffer;
  renderPassInfo.renderArea.extent.width = size.width();
  renderPassInfo.renderArea.extent.width = size.height();

  VkExtent2D extent;
  extent.width = size.width();
  extent.height = size.height();
  renderPassInfo.renderArea.extent = extent;

  renderPassInfo.clearValueCount = 1;
  VkClearValue clearValue;
  clearValue.color = ClearColor;
  renderPassInfo.pClearValues = &clearValue;

  g_vk->vkCmdBeginRenderPass(currentFrame->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void MapRenderer::createRenderPass()
{
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = this->colorFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (g_vk->vkCreateRenderPass(window.device(), &renderPassInfo, nullptr, &this->renderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create render pass!");
  }
}

void MapRenderer::createGraphicsPipeline()
{
  std::vector<uint8_t> vertShaderCode = File::read("shaders/vert.spv");
  std::vector<uint8_t> fragShaderCode = File::read("shaders/frag.spv");

  VkShaderModule vertShaderModule = VulkanHelpers::createShaderModule(window.device(), vertShaderCode);
  VkShaderModule fragShaderModule = VulkanHelpers::createShaderModule(window.device(), fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertShaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragShaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  auto bindingDescription = Vertex::getBindingDescription();
  auto attributeDescriptions = Vertex::getAttributeDescriptions();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
  // This way the pipeline does not need to be touched when resizing the window.
  VkPipelineViewportStateCreateInfo viewportState;
  memset(&viewportState, 0, sizeof(viewportState));
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
      VK_COLOR_COMPONENT_A_BIT;

  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  VkDynamicState dynEnable[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicInfo;
  memset(&dynamicInfo, 0, sizeof(dynamicInfo));
  dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicInfo.dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState);
  dynamicInfo.pDynamicStates = dynEnable;

  std::array<VkDescriptorSetLayout, 2> layouts = {uboDescriptorSetLayout, textureDescriptorSetLayout};
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.size = sizeof(TextureOffset);
  pushConstantRange.offset = 0;

  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (g_vk->vkCreatePipelineLayout(window.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
  {
    qDebug() << "failed to create pipeline layout!";
    throw std::runtime_error("failed to create pipeline layout!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicInfo;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (g_vk->vkCreateGraphicsPipelines(window.device(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  g_vk->vkDestroyShaderModule(window.device(), fragShaderModule, nullptr);
  g_vk->vkDestroyShaderModule(window.device(), vertShaderModule, nullptr);
}

void MapRenderer::createDescriptorSetLayouts()
{
  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;

  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  layoutInfo.pBindings = &uboLayoutBinding;

  if (g_vk->vkCreateDescriptorSetLayout(window.device(), &layoutInfo, nullptr, &uboDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor set layout for the uniform buffer object.");
  }

  VkDescriptorSetLayoutBinding textureLayoutBinding = {};
  textureLayoutBinding.binding = 0;
  textureLayoutBinding.descriptorCount = 1;
  textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureLayoutBinding.pImmutableSamplers = nullptr;
  textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  layoutInfo.pBindings = &textureLayoutBinding;

  if (g_vk->vkCreateDescriptorSetLayout(window.device(), &layoutInfo, nullptr, &textureDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor set layout for the textures.");
  }
}

void MapRenderer::createUniformBuffers()
{
  VkDeviceSize bufferSize = sizeof(ItemUniformBufferObject);

  std::cout << "window.MAX_CONCURRENT_FRAME_COUNT: " << window.MAX_CONCURRENT_FRAME_COUNT << std::endl;
  for (size_t i = 0; i < window.MAX_CONCURRENT_FRAME_COUNT; i++)
  {
    frames[i].uniformBuffer = Buffer::create(bufferSize,
                                             VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  }
}

void MapRenderer::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  uint32_t descriptorCount = window.MAX_CONCURRENT_FRAME_COUNT * 2;

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = descriptorCount;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MAX_NUM_TEXTURES;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = descriptorCount + MAX_NUM_TEXTURES;

  if (g_vk->vkCreateDescriptorPool(window.device(), &poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void MapRenderer::createDescriptorSets()
{
  const uint32_t maxFrames = window.MAX_CONCURRENT_FRAME_COUNT;
  std::vector<VkDescriptorSetLayout> layouts(maxFrames, uboDescriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = maxFrames;
  allocInfo.pSetLayouts = layouts.data();

  std::array<VkDescriptorSet, maxFrames> descriptorSets;

  if (g_vk->vkAllocateDescriptorSets(window.device(), &allocInfo, &descriptorSets[0]) != VK_SUCCESS)
  {
    ABORT_PROGRAM("failed to allocate descriptor sets");
  }

  for (uint32_t i = 0; i < descriptorSets.size(); ++i)
  {
    frames[i].uboDescriptorSet = descriptorSets[i];
  }

  for (size_t i = 0; i < window.MAX_CONCURRENT_FRAME_COUNT; ++i)
  {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = frames[i].uniformBuffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(ItemUniformBufferObject);

    VkWriteDescriptorSet descriptorWrites = {};
    descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites.dstSet = frames[i].uboDescriptorSet;
    descriptorWrites.dstBinding = 0;
    descriptorWrites.dstArrayElement = 0;
    descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites.descriptorCount = 1;
    descriptorWrites.pBufferInfo = &bufferInfo;

    g_vk->vkUpdateDescriptorSets(window.device(), 1, &descriptorWrites, 0, nullptr);
  }
}