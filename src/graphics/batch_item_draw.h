#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <variant>

#include "vertex.h"
#include "../debug.h"

#include "texture.h"
#include "texture_atlas.h"
#include "buffer.h"

#include "vulkan_helpers.h"

#include "../item.h"
#include "../position.h"

constexpr uint32_t BatchDeviceSize = 4 * 128 * sizeof(Vertex);

struct ObjectDrawInfo
{
	Appearance *appearance;
	TextureInfo textureInfo;
	Position position;
	glm::vec4 color{};
	DrawOffset drawOffset = {0, 0};
	VkDescriptorSet descriptorSet;
};

struct RectangleDrawInfo
{
	WorldPosition from;
	WorldPosition to;
	glm::vec4 color{};
	std::variant<Texture *, TextureInfo> texture;
	VkDescriptorSet descriptorSet;
};

struct Batch
{
	struct DescriptorIndex
	{
		VkDescriptorSet descriptor;
		uint32_t end;
	};

	Batch(VulkanInfo *vulkanInfo);
	~Batch();
	BoundBuffer buffer;
	BoundBuffer stagingBuffer;

	Batch(Batch &&other) noexcept;

	Vertex *vertices = nullptr;
	Vertex *current = nullptr;

	uint32_t vertexCount = 0;

	std::vector<DescriptorIndex> descriptorIndices;
	VkDescriptorSet currentDescriptorSet;

	bool isCopiedToDevice = false;

	void setDescriptor(VkDescriptorSet descriptor);

	void addVertex(const Vertex &vertex);

	template <std::size_t SIZE>
	void addVertices(std::array<Vertex, SIZE> &vertices);

	void reset();

	void mapStagingBuffer();
	void unmapStagingBuffer();

	inline bool isValid() const noexcept
	{
		return valid;
	}

	void copyStagingToDevice(VkCommandBuffer commandBuffer);

	bool canHold(uint32_t vertexCount) const
	{
		return (static_cast<uint64_t>(this->vertexCount) + vertexCount) * sizeof(Vertex) < BatchDeviceSize;
	}

	void invalidate();

	friend class BatchDraw;

private:
	// Flag to signal whether recreation (i.e. re-mapping) is necessary.
	bool valid = true;
};

class BatchDraw
{
public:
	VulkanInfo *vulkanInfo;
	VkCommandBuffer commandBuffer;

	BatchDraw(VulkanInfo *vulkanInfo = nullptr);

	void addItem(ObjectDrawInfo &info);
	void addRectangle(RectangleDrawInfo &info);

	Batch &getBatch() const;
	std::vector<Batch> &getBatches() const;
	void prepareDraw();

	void reset();

private:
	mutable uint32_t batchIndex = 0;
	mutable std::vector<Batch> batches;

	Batch &getBatch(uint32_t requiredVertexCount) const;
};
