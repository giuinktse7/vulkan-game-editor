#include "batch_item_draw.h"

#include <ios>
#include "vulkan_helpers.h"

constexpr int MaxDrawOffsetPixels = 24;

BatchDraw::BatchDraw(VulkanInfo *vulkanInfo)
    : vulkanInfo(vulkanInfo), commandBuffer(nullptr), batchIndex(0)
{
}

Batch::Batch(VulkanInfo *vulkanInfo)
    : vertexCount(0), currentDescriptorSet(nullptr), valid(true)
{
  this->stagingBuffer = Buffer::create(
      vulkanInfo,
      BatchDeviceSize,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  // std::cout << "Creating buffer at 0x" << std::hex << this->stagingBuffer.deviceMemory << std::endl;

  this->buffer = Buffer::create(
      vulkanInfo,
      BatchDeviceSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  mapStagingBuffer();
}

Batch::~Batch()
{
  if (current != nullptr)
  {
    unmapStagingBuffer();
  }
}

Batch::Batch(Batch &&other) noexcept
{
  this->valid = other.valid;
  this->isCopiedToDevice = other.isCopiedToDevice;
  this->buffer = std::move(other.buffer);
  this->stagingBuffer = std::move(other.stagingBuffer);
  this->descriptorIndices = other.descriptorIndices;
  this->currentDescriptorSet = other.currentDescriptorSet;
  this->vertexCount = other.vertexCount;
  this->vertices = other.vertices;
  this->current = other.current;

  other.vertexCount = 0;
  other.stagingBuffer.buffer = nullptr;
  other.stagingBuffer.deviceMemory = nullptr;
  other.buffer.buffer = nullptr;
  other.buffer.deviceMemory = nullptr;
  other.currentDescriptorSet = nullptr;
  other.vertices = nullptr;
  other.current = nullptr;
}

void Batch::invalidate()
{
  this->valid = false;
}

void Batch::reset()
{
  if (current != nullptr)
  {
    unmapStagingBuffer();
  }

  this->vertexCount = 0;
  this->descriptorIndices.clear();
  this->currentDescriptorSet = nullptr;
  this->valid = true;

  mapStagingBuffer();
}

void Batch::mapStagingBuffer()
{
  void *data;

  devFuncs()->vkMapMemory(stagingBuffer.vulkanInfo->device(), stagingBuffer.deviceMemory, 0, BatchDeviceSize, 0, &data);
  this->vertices = reinterpret_cast<Vertex *>(data);
  this->current = vertices;

  this->isCopiedToDevice = false;
}

void Batch::unmapStagingBuffer()
{
  DEBUG_ASSERT(current != nullptr, "Tried to unmap an item batch that has not been mapped.");

  devFuncs()->vkUnmapMemory(stagingBuffer.vulkanInfo->device(), stagingBuffer.deviceMemory);
  this->vertices = nullptr;
  this->current = nullptr;
}

void Batch::copyStagingToDevice(VkCommandBuffer commandBuffer)
{
  DEBUG_ASSERT(this->isCopiedToDevice == false, "The staging buffer has alreday been copied to the device.");
  this->isCopiedToDevice = true;

  if (vertexCount == 0)
    return;

  VkBufferCopy copyRegion = {};
  copyRegion.size = this->vertexCount * sizeof(Vertex);
  devFuncs()->vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer.buffer, 1, &copyRegion);
}

void BatchDraw::addItem(ObjectDrawInfo &info)
{
  auto [appearance, textureInfo, position, color, drawOffset, descriptorSet] = info;
  auto [atlas, window] = textureInfo;

  WorldPosition worldPos = MapPosition{position.x + atlas->drawOffset.x, position.y + atlas->drawOffset.y}.worldPos();

  // Add draw offsets like elevation
  worldPos.x += std::clamp(info.drawOffset.x, -MaxDrawOffsetPixels, MaxDrawOffsetPixels);
  worldPos.y += std::clamp(info.drawOffset.y, -MaxDrawOffsetPixels, MaxDrawOffsetPixels);

  // Add the item shift if necessary
  if (appearance->hasFlag(AppearanceFlag::Shift))
  {
    worldPos.x -= appearance->flagData.shiftX;
    worldPos.y -= appearance->flagData.shiftY;
  }

  uint32_t width = atlas->spriteWidth;
  uint32_t height = atlas->spriteHeight;
  const glm::vec4 fragmentBounds = atlas->getFragmentBounds(window);

  Batch &batch = getBatch(4);
  batch.setDescriptor(descriptorSet);

  std::array<Vertex, 4> vertices{{
      {{worldPos.x, worldPos.y}, color, {window.x0, window.y0}, fragmentBounds},
      {{worldPos.x, worldPos.y + height}, color, {window.x0, window.y1}, fragmentBounds},
      {{worldPos.x + width, worldPos.y + height}, color, {window.x1, window.y1}, fragmentBounds},
      {{worldPos.x + width, worldPos.y}, color, {window.x1, window.y0}, fragmentBounds},
  }};

  batch.addVertices(vertices);
}

void BatchDraw::addRectangle(RectangleDrawInfo &info)
{
  Batch &batch = getBatch(4);

  TextureWindow window{};
  glm::vec4 fragmentBounds{};

  DEBUG_ASSERT(std::holds_alternative<Texture *>(info.texture) || std::holds_alternative<TextureInfo>(info.texture), "Unknown texture in addRectangle.");

  if (std::holds_alternative<Texture *>(info.texture))
  {
    window = {0, 0, 1, 1};
    fragmentBounds = {0, 0, 1, 1};
  }
  else if (std::holds_alternative<TextureInfo>(info.texture))
  {
    const TextureInfo textureInfo = std::get<TextureInfo>(info.texture);
    window = textureInfo.window;
    fragmentBounds = textureInfo.atlas->getFragmentBounds(window);
  }

  batch.setDescriptor(info.descriptorSet);

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

  std::array<Vertex, 4> vertices{{
      {{x1, y1}, info.color, {window.x0, window.y0}, fragmentBounds},
      {{x1, y2}, info.color, {window.x0, window.y1}, fragmentBounds},
      {{x2, y2}, info.color, {window.x1, window.y1}, fragmentBounds},
      {{x2, y1}, info.color, {window.x1, window.y0}, fragmentBounds},
  }};

  batch.addVertices(vertices);
}

void BatchDraw::reset()
{
  vulkanInfo = nullptr;
  commandBuffer = VK_NULL_HANDLE;

  batchIndex = 0;
  batches.clear();
}

Batch &BatchDraw::getBatch() const
{
  // Request no additional space
  return getBatch(0);
}

Batch &BatchDraw::getBatch(uint32_t requiredVertexCount) const
{
  if (batches.empty())
  {
    batches.emplace_back(vulkanInfo);
  }

  Batch &batch = batches.at(batchIndex);

  if (!batch.valid)
  {
    batch.reset();
  }

  if (!batch.canHold(requiredVertexCount))
  {
    batch.setDescriptor(nullptr);
    batch.copyStagingToDevice(this->commandBuffer);
    batch.unmapStagingBuffer();

    ++batchIndex;
    if (batchIndex == batches.size())
      batches.emplace_back(vulkanInfo);
  }

  Batch &resultBatch = batches.at(batchIndex);

  if (!resultBatch.valid)
  {
    resultBatch.reset();
  }

  return resultBatch;
}

void Batch::addVertex(const Vertex &vertex)
{
  *current = vertex;
  ++current;
  ++vertexCount;
}

void Batch::setDescriptor(VkDescriptorSet descriptor)
{
  if (this->currentDescriptorSet && this->currentDescriptorSet != descriptor)
  {
    descriptorIndices.emplace_back<Batch::DescriptorIndex>({this->currentDescriptorSet, vertexCount - 1});
  }

  this->currentDescriptorSet = descriptor;
}

void BatchDraw::prepareDraw()
{
  Batch &latestBatch = getBatch();
  if (latestBatch.descriptorIndices.empty() || latestBatch.descriptorIndices.back().descriptor != latestBatch.currentDescriptorSet)
  {
    latestBatch.setDescriptor(nullptr);
  }

  if (!latestBatch.isCopiedToDevice)
  {
    latestBatch.copyStagingToDevice(commandBuffer);
    latestBatch.unmapStagingBuffer();
  }

  this->batchIndex = 0;
}

std::vector<Batch> &BatchDraw::getBatches() const
{
  return batches;
}

template <std::size_t SIZE>
void Batch::addVertices(std::array<Vertex, SIZE> &vertexArray)
{
  memcpy(this->current, &vertexArray, sizeof(vertexArray));
  current += vertexArray.size();
  vertexCount += static_cast<uint32_t>(vertexArray.size());
}