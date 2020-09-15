#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanWindowRenderer>

#include <glm/glm.hpp>

#include <vector>
#include <memory>

#include <unordered_map>

#include "graphics/buffer.h"
#include "graphics/vertex.h"
#include "graphics/texture.h"
#include "graphics/batch_item_draw.h"

#include "item.h"
#include "items.h"

#include "graphics/texture_atlas.h"

#include "map_view.h"

#include "map.h"

class VulkanWindow;
class BatchDraw;

namespace ItemDrawFlags
{
	constexpr uint32_t None = 0;
	constexpr uint32_t DrawSelected = 1 << 0;
} // namespace ItemDrawFlags

struct TextureOffset
{
	float x;
	float y;
};

struct ItemUniformBufferObject
{
	glm::mat4 projection;
};

enum BlendMode
{
	BM_NONE,
	BM_BLEND,
	BM_ADD,
	BM_ADDX2,

	NUM_BLENDMODES
};

struct FrameData
{
	VkFramebuffer frameBuffer = nullptr;
	VkCommandBuffer commandBuffer = nullptr;
	BoundBuffer uniformBuffer;
	VkDescriptorSet uboDescriptorSet = nullptr;

	BatchDraw batchDraw;
};

class TextureResource
{
public:
	struct Descriptor
	{
		VkDescriptorSetLayout layout;
		VkDescriptorPool pool;
	};

	TextureResource();

	bool unused = true;

	void initResources(TextureAtlas &atlas, VulkanInfo &vulkanInfo, TextureResource::Descriptor descriptor);
	void releaseResources();

	inline bool hasResources() const
	{
		return !(textureImage == VK_NULL_HANDLE && textureImageMemory == VK_NULL_HANDLE && _descriptorSet == VK_NULL_HANDLE);
	}

	inline VkDescriptorSet descriptorSet() const
	{
		return _descriptorSet;
	}

private:
	uint32_t width;
	uint32_t height;

	VulkanInfo *vulkanInfo;
	VkImage textureImage = VK_NULL_HANDLE;
	VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
	VkDescriptorSet _descriptorSet = VK_NULL_HANDLE;

	VkDescriptorSet createDescriptorSet(TextureResource::Descriptor descriptor);
	void copyStagingBufferToImage(VkBuffer stagingBuffer);
	VkImageView createImageView(VkImage image, VkFormat format);
	void createImage(VkFormat format,
									 VkImageTiling tiling,
									 VkImageUsageFlags usage,
									 VkMemoryPropertyFlags properties);
	VkSampler createSampler();
};

class MapRenderer : public QVulkanWindowRenderer
{
public:
	MapRenderer(VulkanWindow &window);
	static const int MAX_NUM_TEXTURES = 256 * 256;

	static const int TILE_SIZE = 32;
	static const uint32_t MAX_VERTICES = 64 * 1024;

	void initResources() override;
	void initSwapChainResources() override;
	void releaseSwapChainResources() override;
	void releaseResources() override;

	void startNextFrame() override;

	VkDescriptorPool &getDescriptorPool()
	{
		return descriptorPool;
	}

	VkDescriptorSetLayout &getTextureDescriptorSetLayout()
	{
		return textureDescriptorSetLayout;
	}

	void setMapView(MapView &mapView)
	{
		this->mapView = &mapView;
	}

private:
	bool debug = false;
	bool initialized = false;
	MapView *mapView;
	VulkanWindow &window;
	std::array<FrameData, 3> frames;

	// All sprites are drawn using this index buffer
	BoundBuffer indexBuffer;

	VkFormat colorFormat;

	FrameData *currentFrame;

	VkRenderPass renderPass;

	VkDescriptorPool descriptorPool = 0;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline = 0;

	VkDescriptorSetLayout uboDescriptorSetLayout = 0;
	VkDescriptorSetLayout textureDescriptorSetLayout;

	std::vector<uint32_t> usedTextureAtlasIds;
	std::vector<TextureResource> textureAtlasResources;

	QVulkanDeviceFunctions *devFuncs = nullptr;

	void createRenderPass();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<uint8_t> &code);
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSetLayouts();
	void createDescriptorSets();
	void createIndexBuffer();

	void updateUniformBuffer();

	void beginRenderPass();

	void drawMap();
	void drawPreviewCursor(uint16_t serverId);
	void drawMovingSelection();
	void drawSelectionRectangle();

	void drawTile(const TileLocation &tileLocation, uint32_t drawFlags = ItemDrawFlags::None);
	void drawItem(ObjectDrawInfo &info);

	void drawBatches();
};
