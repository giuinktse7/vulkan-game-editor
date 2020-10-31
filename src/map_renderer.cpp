#include "map_renderer.h"

#include <stdexcept>
#include <variant>

#include "file.h"
#include "position.h"

#include "graphics/appearances.h"

#include "ecs/ecs.h"
#include "debug.h"
#include "util.h"
#include "logger.h"

/** Order of members matter for this struct due to alignment requirements in the 
 * vertex shader.
 * 
 * See: Vulkan Spec: 14.5.4. Offset and Stride Assignment
 */
struct PushConstantData
{
  glm::vec4 textureQuad;
  glm::vec4 fragQuad;
  glm::vec4 color;
  glm::vec4 pos;
  glm::vec4 size;
};

struct NewVertex
{
  glm::ivec2 position;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bindingDescription{};

    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(NewVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions()
  {
    std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;

    // attributeDescriptions[4].binding = 0;
    // attributeDescriptions[4].location = 4;
    // attributeDescriptions[4].format = VK_FORMAT_R8_UINT;
    // attributeDescriptions[4].offset = offsetof(Vertex, useUbo);

    return attributeDescriptions;
  }
};

constexpr int GROUND_FLOOR = 7;

constexpr VkFormat ColorFormat = VK_FORMAT_B8G8R8A8_UNORM;

constexpr VkClearColorValue ClearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};

// A rectangle is drawn using two triangles, each with 3 vertex indices.
constexpr uint32_t IndexBufferSize = 6 * sizeof(uint16_t);
constexpr uint32_t VertexBufferSize = 4 * sizeof(NewVertex);

glm::vec4 colors::opacity(float value)
{
  DEBUG_ASSERT(0 <= value && value <= 1, "value must be in range [0.0f, 1.0f].");
  return glm::vec4(1.0f, 1.0f, 1.0f, value);
}

MapRenderer::MapRenderer(VulkanInfo &vulkanInfo, MapView *mapView)
    : mapView(mapView),
      vulkanInfo(vulkanInfo),
      vulkanTexturesForAppearances(Appearances::textureAtlasCount()),
      vulkanSwapChainImageSize(0, 0)
{
  activeTextureAtlasIds.reserve(Appearances::textureAtlasCount());

  size_t ArbitraryGeneralReserveAmount = 8;
  vulkanTextures.reserve(ArbitraryGeneralReserveAmount);
}

void MapRenderer::initResources(VkFormat colorFormat)
{
  // VME_LOG_D("[window: " << window.debugName << "] MapRenderer::initResources (device: " << window.device() << ")");

  vulkanInfo.update();
  this->colorFormat = colorFormat;

  createRenderPass();

  _currentFrame = &frames.front();

  createDescriptorSetLayouts();
  createGraphicsPipeline();
  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();
  createVertexBuffer();
  createIndexBuffer();

  // VME_LOG_D("End MapRenderer::initResources");
}

void MapRenderer::initSwapChainResources(util::Size vulkanSwapChainImageSize)
{
  mapView->setViewportSize(vulkanSwapChainImageSize.width(), vulkanSwapChainImageSize.height());
}

void MapRenderer::releaseSwapChainResources()
{
  mapView->setViewportSize(0, 0);
}

void MapRenderer::releaseResources()
{
  // auto device = window.device();
  // VME_LOG_D("[window: " << window.debugName << "] MapRenderer::releaseResources (device: " << device << ")");

  vulkanInfo.vkDestroyDescriptorSetLayout(uboDescriptorSetLayout, nullptr);
  uboDescriptorSetLayout = VK_NULL_HANDLE;

  vulkanInfo.vkDestroyDescriptorSetLayout(textureDescriptorSetLayout, nullptr);
  textureDescriptorSetLayout = VK_NULL_HANDLE;

  vulkanInfo.vkDestroyDescriptorPool(descriptorPool, nullptr);
  descriptorPool = VK_NULL_HANDLE;

  vulkanInfo.vkDestroyPipeline(graphicsPipeline, nullptr);
  graphicsPipeline = VK_NULL_HANDLE;

  vulkanInfo.vkDestroyPipelineLayout(pipelineLayout, nullptr);
  pipelineLayout = VK_NULL_HANDLE;

  vulkanInfo.vkDestroyRenderPass(renderPass, nullptr);
  renderPass = VK_NULL_HANDLE;

  vertexBuffer.releaseResources();
  indexBuffer.releaseResources();

  for (const auto id : activeTextureAtlasIds)
  {
    vulkanTexturesForAppearances.at(id).releaseResources();
  }
  activeTextureAtlasIds.clear();

  vulkanTextures.clear();

  for (auto &frame : frames)
  {
    frame.uniformBuffer = {};
    frame.commandBuffer = VK_NULL_HANDLE;
    frame.frameBuffer = VK_NULL_HANDLE;
    frame.uboDescriptorSet = VK_NULL_HANDLE;
  }

  debug = true;
}

void MapRenderer::startNextFrame()
{
  VME_LOG("Start next frame");
  g_ecs.getSystem<ItemAnimationSystem>().update();

  mapView->updateViewport();

  updateUniformBuffer();

  beginRenderPass();

  setupFrame();

  drawMap();
  drawCurrentAction();

  vulkanInfo.vkCmdEndRenderPass(_currentFrame->commandBuffer);

  vulkanInfo.frameReady();
  // vulkanInfo.requestUpdate();
  currentDescriptorSet = nullptr;
}

void MapRenderer::setupFrame()
{
  vulkanInfo.vkCmdBindPipeline(_currentFrame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

  const util::Size size = vulkanInfo.vulkanSwapChainImageSize();

  VkViewport viewport;
  viewport.x = viewport.y = 0;
  viewport.width = size.width();
  viewport.height = size.height();
  viewport.minDepth = 0;
  viewport.maxDepth = 1;
  vulkanInfo.vkCmdSetViewport(_currentFrame->commandBuffer, 0, 1, &viewport);

  VkRect2D scissor;
  scissor.offset.x = scissor.offset.y = 0;
  scissor.extent.width = viewport.width;
  scissor.extent.height = viewport.height;
  vulkanInfo.vkCmdSetScissor(_currentFrame->commandBuffer, 0, 1, &scissor);

  VkDeviceSize offsets[] = {0};

  vulkanInfo.vkCmdBindDescriptorSets(
      _currentFrame->commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelineLayout,
      0,
      1,
      &_currentFrame->uboDescriptorSet,
      0,
      nullptr);

  vulkanInfo.vkCmdBindVertexBuffers(_currentFrame->commandBuffer, 0, 1, &vertexBuffer.buffer, offsets);
  vulkanInfo.vkCmdBindIndexBuffer(_currentFrame->commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
}

void MapRenderer::drawMap()
{
  MapView &view = *mapView;
  const auto mapRect = view.getGameBoundingRect();
  int floor = view.z();

  bool aboveGround = floor <= 7;

  int startZ = aboveGround ? GROUND_FLOOR : MAP_LAYERS - 1;
  int endZ = floor;

  Position from{mapRect.x1, mapRect.y1, startZ};
  Position to{mapRect.x2, mapRect.y2, endZ};

  ItemPredicate filter = nullptr;
  if (mapView->draggingWithSubtract())
  {
    auto [from, to] = mapView->getDragPoints().value();
    Region2D dragRegion(from.toPos(floor), to.toPos(floor));
    uint16_t serverId = std::get<MouseAction::RawItem>(mapView->editorAction.action()).serverId;

    filter = [serverId, &dragRegion](const Position pos, const Item &item) {
      return !(item.serverId() == serverId && dragRegion.contains(pos));
    };
  }

  bool movingSelection = view.selection().moving();

  uint32_t flags = ItemDrawFlags::DrawNonSelected;

  if (!movingSelection)
    flags |= ItemDrawFlags::DrawSelected;

  auto selectAction = mapView->editorAction.as<MouseAction::Select>();
  if (selectAction && selectAction->area)
    flags |= ItemDrawFlags::ActiveSelectionArea;

  for (auto &tileLocation : view.map()->getRegion(from, to))
  {
    if (!tileLocation.hasTile() || (movingSelection && tileLocation.tile()->allSelected()))
      continue;

    drawTile(tileLocation, flags);
  }
}

void MapRenderer::drawCurrentAction()
{
  // Render current mouse action
  std::visit(
      util::overloaded{
          [this](const MouseAction::Select select) {
            if (select.area)
            {
              DEBUG_ASSERT(mapView->isDragging(), "action.area == true is invalid if no drag is active.");

              // const auto [from, to] = mapView->getDragPoints().value();
              // drawSolidRectangle(SolidColor::Blue, from, to, 0.1f);
            }
            else if (mapView->selection().moving())
            {
              drawMovingSelection();
            }
          },

          [this](const MouseAction::RawItem &action) {
            if (_currentFrame->mouseHover)
            {
              Position pos = mapView->mouseGamePos();

              if (action.area)
              {
                DEBUG_ASSERT(mapView->isDragging(), "action.area == true is invalid if no drag is active.");

                if (mapView->draggingWithSubtract())
                {
                  const auto [from, to] = mapView->getDragPoints().value();
                  drawSolidRectangle(SolidColor::Red, from, to, 0.2f);

                  drawOverlayItemType(action.serverId, to);
                }
                else
                {
                  auto [from, to] = mapView->getDragPoints().value();
                  int floor = mapView->floor();
                  auto area = MapArea(from.toPos(floor), to.toPos(floor));

                  for (auto &pos : area)
                    drawPreviewItem(action.serverId, pos);
                }
              }
              else
              {
                drawPreviewItem(action.serverId, pos);
              }
            }
          },

          [](const auto &arg) {}},
      _currentFrame->mouseAction);
}

void MapRenderer::drawPreviewItem(uint16_t serverId, Position pos)
{
  auto map = mapView->map();
  if (pos.x < 0 || pos.x > map->width() || pos.y < 0 || pos.y > map->height())
    return;

  ItemType &selectedItemType = *Items::items.getItemType(serverId);

  auto info = itemTypeDrawInfo(selectedItemType, pos, ItemDrawFlags::Ghost);

  if (!selectedItemType.isGroundTile())
  {
    Tile *tile = map->getTile(pos);
    int elevation = tile ? tile->getTopElevation() : 0;
    info.drawOffset = {-elevation, -elevation};
  }

  drawItem(info);
}

void MapRenderer::drawMovingSelection()
{
  auto mapRect = mapView->getGameBoundingRect();

  Position moveDelta = mapView->selection().moveDelta();

  mapRect = mapRect.translate(-moveDelta.x, -moveDelta.y, {0, 0});

  // TODO: Use selection Z bounds instead of all floors
  int startZ = MAP_LAYERS - 1;
  int endZ = 0;

  Position from(mapRect.x1, mapRect.y1, startZ);
  Position to(mapRect.x2, mapRect.y2, endZ);

  for (auto &tileLocation : mapView->map()->getRegion(from, to))
  {
    if (tileLocation.hasTile())
    {
      // Draw only if the tile has a selection.
      if (tileLocation.tile()->hasSelection())
      {
        drawTile(tileLocation, ItemDrawFlags::DrawSelected, moveDelta);
      }
    }
  }
}

bool MapRenderer::shouldDrawItem(const Position pos, const Item &item, uint32_t flags, const ItemPredicate &filter) const noexcept
{
  return ((item.selected && (flags & ItemDrawFlags::DrawSelected)) ||
          (flags & ItemDrawFlags::DrawNonSelected)) &&
         (!filter || filter(pos, item));
}

void MapRenderer::drawTile(const TileLocation &tileLocation, uint32_t flags, const ItemPredicate &filter)
{
  drawTile(tileLocation, flags, PositionConstants::Zero, filter);
}

void MapRenderer::drawTile(const TileLocation &tileLocation, uint32_t flags, const Position offset, const ItemPredicate &filter)
{
  auto position = tileLocation.position();
  position += offset;
  auto tile = tileLocation.tile();

  Item *groundPtr = tile->ground();
  if (groundPtr != nullptr)
  {
    if (shouldDrawItem(position, *groundPtr, flags, filter))
    {
      auto info = itemDrawInfo(*groundPtr, position, flags);
      drawItem(info);
    }
  }

  DrawOffset drawOffset{0, 0};
  for (const Item &item : tile->items())
  {
    if (!shouldDrawItem(position, item, flags, filter))
      continue;

    auto info = itemDrawInfo(item, position, flags);
    info.drawOffset = drawOffset;
    drawItem(info);

    if (item.itemType->hasElevation())
    {
      uint32_t elevation = item.itemType->getElevation();
      drawOffset.x -= elevation;
      drawOffset.y -= elevation;
    }
  }
}

void MapRenderer::drawItem(const DrawInfo::Object &info)
{
  constexpr int MaxDrawOffsetPixels = 24;
  const auto *atlas = info.textureInfo.atlas;

  WorldPosition worldPos = MapPosition{info.position.x + atlas->drawOffset.x, info.position.y + atlas->drawOffset.y}.worldPos();

  // Add draw offsets like elevation
  worldPos.x += std::clamp(info.drawOffset.x, -MaxDrawOffsetPixels, MaxDrawOffsetPixels);
  worldPos.y += std::clamp(info.drawOffset.y, -MaxDrawOffsetPixels, MaxDrawOffsetPixels);

  // Add the item shift if necessary
  if (info.appearance->hasFlag(AppearanceFlag::Shift))
  {
    worldPos.x -= info.appearance->flagData.shiftX;
    worldPos.y -= info.appearance->flagData.shiftY;
  }

  const auto &window = info.textureInfo.window;
  PushConstantData pushConstant{};

  glm::vec4 pos{};
  pos.x = worldPos.x;
  pos.y = worldPos.y;
  pushConstant.pos = pos;

  glm::vec4 size{};
  size.x = atlas->spriteWidth;
  size.y = atlas->spriteHeight;
  pushConstant.size = size;
  pushConstant.color = info.color;
  pushConstant.textureQuad = glm::vec4(window.x0, window.y0, window.x1, window.y1);
  pushConstant.fragQuad = atlas->getFragmentBounds(window);

  vulkanInfo.vkCmdBindDescriptorSets(
      _currentFrame->commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelineLayout,
      1,
      1,
      &info.descriptorSet,
      0,
      nullptr);

  vulkanInfo.vkCmdPushConstants(_currentFrame->commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pushConstant);
  vulkanInfo.vkCmdDrawIndexed(_currentFrame->commandBuffer, 6, 1, 0, 0, 0);
}

void MapRenderer::drawOverlayItemType(uint16_t serverId, const WorldPosition position, const glm::vec4 color)
{
  ItemType &itemType = *Items::items.getItemType(serverId);
  DrawInfo::OverlayObject info;
  info.appearance = itemType.appearance;
  info.position = position;
  info.color = color;
  info.textureInfo = itemType.getTextureInfo();
  info.descriptorSet = objectDescriptorSet(info);

  // _currentFrame->batchDraw.addOverlayItem(info);
}

void MapRenderer::drawRectangle(DrawInfo::Rectangle &info)
{
  PushConstantData pushConstant{};

  if (std::holds_alternative<const Texture *>(info.texture))
  {
    pushConstant.textureQuad = {0, 0, 1, 1};
    pushConstant.fragQuad = {0, 0, 1, 1};
  }
  else if (std::holds_alternative<TextureInfo>(info.texture))
  {
    const TextureInfo textureInfo = std::get<TextureInfo>(info.texture);
    pushConstant.textureQuad = textureInfo.window.asVec4();
    pushConstant.fragQuad = textureInfo.atlas->getFragmentBounds(textureInfo.window);
  }

  auto [x1, y1] = info.from;
  auto [x2, y2] = info.to;

  // Handle the possible quadrants to draw the texture correctly
  if (x1 > x2)
  {
    std::swap(x1, x2);
    if (y1 > y2)
    {
      std::swap(y1, y2);
    }
  }
  else if (x1 < x2 && y1 > y2)
  {
    std::swap(y1, y2);
  }

  glm::vec4 pos{};
  pos.x = x1;
  pos.y = y1;
  pushConstant.pos = pos;

  glm::vec4 size{};
  size.x = std::abs(x2 - x1);
  size.y = std::abs(y2 - y1);
  pushConstant.size = size;
  pushConstant.color = info.color;

  vulkanInfo.vkCmdBindDescriptorSets(
      _currentFrame->commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipelineLayout,
      1,
      1,
      &info.descriptorSet,
      0,
      nullptr);

  vulkanInfo.vkCmdPushConstants(_currentFrame->commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData), &pushConstant);
  vulkanInfo.vkCmdDrawIndexed(_currentFrame->commandBuffer, 6, 1, 0, 0, 0);
}

void MapRenderer::drawSolidRectangle(const SolidColor color, const WorldPosition from, const WorldPosition to, float opacity)
{
  Texture &texture = Texture::getOrCreateSolidTexture(color);
  drawRectangle(texture, from, to, opacity);
}

void MapRenderer::drawRectangle(const Texture &texture, const WorldPosition from, const WorldPosition to, float opacity)
{
  VulkanTexture::Descriptor descriptor;
  descriptor.layout = textureDescriptorSetLayout;
  descriptor.pool = descriptorPool;

  auto &vulkanTexture = vulkanTextures[&texture];
  if (!vulkanTexture.hasResources())
    vulkanTexture.initResources(texture, vulkanInfo, descriptor);

  DrawInfo::Rectangle info;
  info.from = from;
  info.to = to;
  info.texture = &texture;
  info.color = colors::opacity(opacity);
  info.descriptorSet = vulkanTexture.descriptorSet();

  drawRectangle(info);
}

DrawInfo::Object MapRenderer::itemDrawInfo(const Item &item, Position position, uint32_t drawFlags)
{
  bool drawAsSelected = item.selected || ((drawFlags & ItemDrawFlags::ActiveSelectionArea) && mapView->inDragRegion(position));

  DrawInfo::Object info;
  info.appearance = item.itemType->appearance;
  info.position = position;
  info.color = drawAsSelected ? colors::Selected : colors::Default;
  info.textureInfo = item.getTextureInfo(position);
  info.descriptorSet = objectDescriptorSet(info);

  return info;
}

DrawInfo::Object MapRenderer::itemTypeDrawInfo(const ItemType &itemType, Position position, uint32_t drawFlags)
{
  DrawInfo::Object info;
  info.appearance = itemType.appearance;
  info.position = position;
  info.color = drawFlags & ItemDrawFlags::Ghost ? colors::ItemPreview : colors::Default;
  info.textureInfo = itemType.getTextureInfo(position);
  info.descriptorSet = objectDescriptorSet(info);

  return info;
}

VkDescriptorSet MapRenderer::objectDescriptorSet(const DrawInfo::Base &info)
{
  VulkanTexture::Descriptor descriptor;
  descriptor.layout = textureDescriptorSetLayout;
  descriptor.pool = descriptorPool;

  TextureAtlas *atlas = info.textureInfo.atlas;
  VulkanTexture &vulkanTexture = vulkanTexturesForAppearances.at(atlas->id());

  if (!vulkanTexture.hasResources())
  {
    if (vulkanTexture.unused)
    {
      activeTextureAtlasIds.emplace_back(atlas->id());
    }

    vulkanTexture.initResources(*atlas, vulkanInfo, descriptor);
  }

  return vulkanTexture.descriptorSet();
}

void MapRenderer::updateUniformBuffer()
{
  glm::mat4 projection = vulkanInfo.projectionMatrix(*mapView);
  // const auto p = projection;
  // std::ostringstream s;
  // VME_LOG_D("Next:");
  // for (int i = 0; i < 4; ++i)
  // {
  //   s << "x: " << p[i].x << ", ";
  //   s << "y: " << p[i].y << ", ";
  //   s << "z: " << p[i].z << ", ";
  //   s << "w: " << p[i].w << ", ";
  //   s << std::endl;
  // }
  // VME_LOG_D(s.str());
  ItemUniformBufferObject uniformBufferObject{projection};

  void *data;
  vulkanInfo.vkMapMemory(_currentFrame->uniformBuffer.deviceMemory, 0, sizeof(ItemUniformBufferObject), 0, &data);
  memcpy(data, &uniformBufferObject, sizeof(ItemUniformBufferObject));
  vulkanInfo.vkUnmapMemory(_currentFrame->uniformBuffer.deviceMemory);
}

/**
 * 
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * Vulkan rendering setup/teardown
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 *
 **/
void MapRenderer::beginRenderPass()
{
  util::Size size = vulkanInfo.vulkanSwapChainImageSize();
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = _currentFrame->frameBuffer;
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

  vulkanInfo.vkCmdBeginRenderPass(_currentFrame->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
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

  if (vulkanInfo.vkCreateRenderPass(&renderPassInfo, nullptr, &this->renderPass) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create render pass!");
  }
}

void MapRenderer::createGraphicsPipeline()
{
  std::vector<uint8_t> vertShaderCode = File::read("shaders/vert.spv");
  std::vector<uint8_t> fragShaderCode = File::read("shaders/frag.spv");

  VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
  VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

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

  auto bindingDescription = NewVertex::getBindingDescription();
  auto attributeDescriptions = NewVertex::getAttributeDescriptions();

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
  // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantData);

  std::array<VkDescriptorSetLayout, 2> layouts = {uboDescriptorSetLayout, textureDescriptorSetLayout};
  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vulkanInfo.vkCreatePipelineLayout(&pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
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

  if (vulkanInfo.vkCreateGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
      VK_SUCCESS)
  {
    throw std::runtime_error("failed to create graphics pipeline!");
  }

  vulkanInfo.vkDestroyShaderModule(fragShaderModule, nullptr);
  vulkanInfo.vkDestroyShaderModule(vertShaderModule, nullptr);
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

  if (vulkanInfo.vkCreateDescriptorSetLayout(&layoutInfo, nullptr, &uboDescriptorSetLayout) != VK_SUCCESS)
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

  if (vulkanInfo.vkCreateDescriptorSetLayout(&layoutInfo, nullptr, &textureDescriptorSetLayout) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create descriptor set layout for the textures.");
  }
}

void MapRenderer::createUniformBuffers()
{
  VkDeviceSize bufferSize = sizeof(ItemUniformBufferObject);

  Buffer::CreateInfo info;
  info.vulkanInfo = &vulkanInfo;
  info.size = bufferSize;
  info.usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  info.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  for (size_t i = 0; i < vulkanInfo.maxConcurrentFrameCount(); i++)
  {
    frames[i].uniformBuffer = Buffer::create(info);
  }
}

void MapRenderer::createDescriptorPool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  uint32_t descriptorCount = vulkanInfo.maxConcurrentFrameCount() * 2;

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = descriptorCount;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MAX_NUM_TEXTURES;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = descriptorCount + MAX_NUM_TEXTURES;

  if (vulkanInfo.vkCreateDescriptorPool(&poolInfo, nullptr, &this->descriptorPool) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create descriptor pool!");
  }
}

void MapRenderer::createDescriptorSets()
{
  const uint32_t maxFrames = 3;
  std::vector<VkDescriptorSetLayout> layouts(maxFrames, uboDescriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = maxFrames;
  allocInfo.pSetLayouts = layouts.data();

  std::array<VkDescriptorSet, maxFrames> descriptorSets;

  if (vulkanInfo.vkAllocateDescriptorSets(&allocInfo, &descriptorSets[0]) != VK_SUCCESS)
  {
    ABORT_PROGRAM("failed to allocate descriptor sets");
  }

  for (uint32_t i = 0; i < descriptorSets.size(); ++i)
  {
    frames[i].uboDescriptorSet = descriptorSets[i];
  }

  for (size_t i = 0; i < vulkanInfo.maxConcurrentFrameCount(); ++i)
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

    vulkanInfo.vkUpdateDescriptorSets(1, &descriptorWrites, 0, nullptr);
  }
}

void MapRenderer::createVertexBuffer()
{
  std::array<glm::ivec2, 4> vertices{{{0, 0}, {0, 1}, {1, 1}, {1, 0}}};

  Buffer::CreateInfo stagingInfo;
  stagingInfo.vulkanInfo = &vulkanInfo;
  stagingInfo.size = VertexBufferSize;
  stagingInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingInfo.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  BoundBuffer stagingBuffer = Buffer::create(stagingInfo);

  void *data;
  vulkanInfo.vkMapMemory(stagingBuffer.deviceMemory, 0, VertexBufferSize, 0, &data);
  memcpy(data, &vertices, sizeof(vertices));

  Buffer::CreateInfo bufferInfo;
  bufferInfo.vulkanInfo = &vulkanInfo;
  bufferInfo.size = VertexBufferSize;
  bufferInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  vertexBuffer = Buffer::create(bufferInfo);

  VkCommandBuffer commandBuffer = vulkanInfo.beginSingleTimeCommands();
  VkBufferCopy copyRegion = {};
  copyRegion.size = VertexBufferSize;
  vulkanInfo.vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, vertexBuffer.buffer, 1, &copyRegion);
  vulkanInfo.endSingleTimeCommands(commandBuffer);

  vulkanInfo.vkUnmapMemory(stagingBuffer.deviceMemory);
}

void MapRenderer::createIndexBuffer()
{
  std::array<uint16_t, 6> indices{0, 1, 3, 3, 1, 2};
  Buffer::CreateInfo stagingInfo;
  stagingInfo.vulkanInfo = &vulkanInfo;
  stagingInfo.size = IndexBufferSize;
  stagingInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingInfo.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  BoundBuffer indexStagingBuffer = Buffer::create(stagingInfo);

  void *data;
  vulkanInfo.vkMapMemory(indexStagingBuffer.deviceMemory, 0, IndexBufferSize, 0, &data);
  memcpy(data, &indices, sizeof(indices));

  Buffer::CreateInfo bufferInfo;
  bufferInfo.vulkanInfo = &vulkanInfo;
  bufferInfo.size = IndexBufferSize;
  bufferInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  bufferInfo.memoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  indexBuffer = Buffer::create(bufferInfo);

  VkCommandBuffer commandBuffer = vulkanInfo.beginSingleTimeCommands();
  VkBufferCopy copyRegion = {};
  copyRegion.size = IndexBufferSize;
  vulkanInfo.vkCmdCopyBuffer(commandBuffer, indexStagingBuffer.buffer, indexBuffer.buffer, 1, &copyRegion);
  vulkanInfo.endSingleTimeCommands(commandBuffer);

  vulkanInfo.vkUnmapMemory(indexStagingBuffer.deviceMemory);
}

VkShaderModule MapRenderer::createShaderModule(const std::vector<uint8_t> &code)
{
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule;
  if (vulkanInfo.vkCreateShaderModule(&createInfo, nullptr, &shaderModule) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}

VkCommandBuffer MapRenderer::beginSingleTimeCommands(VulkanInfo *info)
{
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = info->graphicsCommandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  info->vkAllocateCommandBuffers(&allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  info->vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

uint32_t MapRenderer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vulkanInfo.vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
  {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
    {
      return i;
    }
  }

  throw std::runtime_error("Failed to find suitable memory type!");
}

/**
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * VulkanTexture
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
 **/
VulkanTexture::VulkanTexture()
{
}

VulkanTexture::~VulkanTexture()
{
  if (hasResources())
  {
    releaseResources();
  }
}

void VulkanTexture::initResources(TextureAtlas &atlas, VulkanInfo &vulkanInfo, const VulkanTexture::Descriptor descriptor)
{
  initResources(atlas.getOrCreateTexture(), vulkanInfo, descriptor);
}

void VulkanTexture::initResources(const Texture &texture, VulkanInfo &vulkanInfo, const VulkanTexture::Descriptor descriptor)
{
  unused = false;

  width = texture.width();
  height = texture.height();
  uint32_t sizeInBytes = texture.sizeInBytes();

  this->vulkanInfo = &vulkanInfo;
  Buffer::CreateInfo bufferInfo;
  bufferInfo.vulkanInfo = &vulkanInfo;
  bufferInfo.size = sizeInBytes;
  bufferInfo.usageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferInfo.memoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  auto stagingBuffer = Buffer::create(bufferInfo);

  Buffer::copyToMemory(&vulkanInfo, stagingBuffer.deviceMemory, texture.pixels().data(), sizeInBytes);

  createImage(
      ColorFormat,
      VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vulkanInfo.transitionImageLayout(textureImage,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copyStagingBufferToImage(stagingBuffer.buffer);

  vulkanInfo.transitionImageLayout(textureImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  _descriptorSet = createDescriptorSet(descriptor);
}

void VulkanTexture::releaseResources()
{
  DEBUG_ASSERT(hasResources(), "Tried to release resources, but there are no resources in the Texture Resource.");

  vulkanInfo->vkDestroyImage(textureImage, nullptr);
  vulkanInfo->vkFreeMemory(textureImageMemory, nullptr);

  textureImage = VK_NULL_HANDLE;
  _descriptorSet = VK_NULL_HANDLE;
  textureImageMemory = VK_NULL_HANDLE;

  vulkanInfo = nullptr;

  unused = true;
}

void VulkanTexture::createImage(
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vulkanInfo->vkCreateImage(&imageInfo, nullptr, &textureImage) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create image!");
  }

  VkMemoryRequirements memRequirements;
  vulkanInfo->vkGetImageMemoryRequirements(textureImage, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = vulkanInfo->findMemoryType(vulkanInfo->physicalDevice(), memRequirements.memoryTypeBits, properties);

  if (vulkanInfo->vkAllocateMemory(&allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate image memory!");
  }

  vulkanInfo->vkBindImageMemory(textureImage, textureImageMemory, 0);
}

void VulkanTexture::copyStagingBufferToImage(VkBuffer stagingBuffer)
{
  VkCommandBuffer commandBuffer = vulkanInfo->beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vulkanInfo->vkCmdCopyBufferToImage(
      commandBuffer,
      stagingBuffer,
      textureImage,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &region);

  vulkanInfo->endSingleTimeCommands(commandBuffer);
}

VkSampler VulkanTexture::createSampler()
{
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;

  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;

  VkSampler sampler;
  if (vulkanInfo->vkCreateSampler(&samplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create texture sampler!");
  }

  return sampler;
}

VkDescriptorSet VulkanTexture::createDescriptorSet(VulkanTexture::Descriptor descriptor)
{
  VkImageView imageView = createImageView(textureImage, ColorFormat);
  VkSampler sampler = createSampler();

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptor.pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &descriptor.layout;

  if (vulkanInfo->vkAllocateDescriptorSets(&allocInfo, &_descriptorSet) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to allocate texture descriptor set");
  }

  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = imageView;
  imageInfo.sampler = sampler;

  VkWriteDescriptorSet descriptorWrites = {};
  descriptorWrites.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites.dstSet = _descriptorSet;
  descriptorWrites.dstBinding = 0;
  descriptorWrites.dstArrayElement = 0;
  descriptorWrites.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites.descriptorCount = 1;
  descriptorWrites.pImageInfo = &imageInfo;

  vulkanInfo->vkUpdateDescriptorSets(1, &descriptorWrites, 0, nullptr);

  return _descriptorSet;
}

VkImageView VulkanTexture::createImageView(VkImage image, VkFormat format)
{
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if (vulkanInfo->vkCreateImageView(&viewInfo, nullptr, &imageView) != VK_SUCCESS)
  {
    throw std::runtime_error("failed to create texture image view!");
  }

  return imageView;
}