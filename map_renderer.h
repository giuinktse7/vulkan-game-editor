#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanWindow>

#include <glm/glm.hpp>

#include <vector>
#include <memory>

#include <unordered_map>

#include "graphics/buffer.h"
#include "graphics/vertex.h"
#include "graphics/texture.h"
#include "camera.h"

#include "position.h"

#include "item.h"
#include "items.h"

#include "graphics/swapchain.h"

#include "graphics/texture_atlas.h"

#include "graphics/batch_item_draw.h"
#include "map_view.h"

#include "map.h"

class VulkanWindow;

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
	BoundBuffer uniformBuffer{};
	VkDescriptorSet uboDescriptorSet = nullptr;

	BatchDraw batchDraw{};
};

class MapRenderer : public QVulkanWindowRenderer
{
public:
	MapRenderer(VulkanWindow &window);
	static const int MAX_NUM_TEXTURES = 256 * 256;

	static const int TILE_SIZE = 32;
	static const uint32_t MAX_VERTICES = 64 * 1024;

	// All sprites are drawn using this index buffer
	BoundBuffer indexBuffer;

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

	VkFormat colorFormat;

	FrameData *currentFrame;

	VkRenderPass renderPass;

	VkDescriptorPool descriptorPool = 0;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline = 0;

	VkDescriptorSetLayout uboDescriptorSetLayout = 0;
	VkDescriptorSetLayout textureDescriptorSetLayout;

	std::vector<Texture *> activeTextures;

	void createRenderPass();
	void createGraphicsPipeline();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSetLayouts();
	void createDescriptorSets();
	void createFrameBuffers();

	void updateUniformBuffer();

	void beginRenderPass();

	void drawMap();
	void drawPreviewCursor();
	void drawMovingSelection();
	void drawSelectionRectangle();

	void drawTile(const TileLocation &tileLocation, uint32_t drawFlags = ItemDrawFlags::None);
	void drawItem(ObjectDrawInfo &info);

	void drawBatches();

	void drawTestRectangle();
};
