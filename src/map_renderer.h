#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include <vector>
#include <memory>

#include <unordered_map>

#include "graphics/buffer.h"
#include "graphics/vertex.h"
#include "graphics/texture.h"
#include "graphics/texture_atlas.h"
#include "graphics/vulkan_helpers.h"

#include "item.h"
#include "items.h"

#include "map_view.h"
#include "util.h"
#include "input.h"

#include "map.h"

namespace DrawInfo
{
	struct Base
	{
		Appearance *appearance;
		TextureInfo textureInfo;
		glm::vec4 color{};
		VkDescriptorSet descriptorSet;
	};

	/**
 * Describes an overlay object.
 * NOTE: Do not use this to draw a map item. For that, use ObjectDrawInfo;
 * it takes a Position instead of a WorldPosition.
*/
	struct OverlayObject : Base
	{
		WorldPosition position;
	};

	struct Object : Base
	{
		Position position;
		DrawOffset drawOffset = {0, 0};
	};

	struct Vertex
	{
		TextureInfo textureInfo;
		WorldPosition position;
		glm::vec4 color{};
		VkDescriptorSet descriptorSet;
	};

	struct Rectangle
	{
		WorldPosition from;
		WorldPosition to;
		glm::vec4 color{};
		std::variant<const Texture *, TextureInfo> texture;
		VkDescriptorSet descriptorSet;
	};

}; // namespace DrawInfo

namespace colors
{
	constexpr glm::vec4 Default{1.0f, 1.0f, 1.0f, 1.0f};
	constexpr glm::vec4 Selected{0.45f, 0.45f, 0.45f, 1.0f};
	constexpr glm::vec4 Red{1.0f, 0.0f, 0.0f, 1.0f};
	constexpr glm::vec4 SeeThrough{1.0f, 1.0f, 1.0f, 0.35f};
	constexpr glm::vec4 ItemPreview{0.6f, 0.6f, 0.6f, 0.7f};

	glm::vec4 opacity(float value);

} // namespace colors

namespace ItemDrawFlags
{
	constexpr uint32_t None = 0;
	constexpr uint32_t DrawNonSelected = 1 << 0;
	constexpr uint32_t DrawSelected = 1 << 1;
	constexpr uint32_t Ghost = 1 << 2;
	constexpr uint32_t ActiveSelectionArea = 1 << 3;
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

	int currentFrameIndex = 0;

	glm::mat4 projectionMatrix{};
	MouseAction_t mouseAction = MouseAction::None{};
	bool mouseHover = false;
};

/*
	A VulkanTexture contains Vulkan resources that are used to draw a Texture
	to the screen.
*/
class VulkanTexture
{
public:
	struct Descriptor
	{
		VkDescriptorSetLayout layout;
		VkDescriptorPool pool;
	};

	VulkanTexture();
	~VulkanTexture();

	VulkanTexture(const VulkanTexture &other) = delete;
	VulkanTexture &operator=(const ItemType &other) = delete;

	bool unused = true;

	void initResources(TextureAtlas &atlas, VulkanInfo &vulkanInfo, const VulkanTexture::Descriptor descriptor);
	void initResources(const Texture &texture, VulkanInfo &vulkanInfo, const VulkanTexture::Descriptor descriptor);
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

	VkDescriptorSet createDescriptorSet(VulkanTexture::Descriptor descriptor);
	void copyStagingBufferToImage(VkBuffer stagingBuffer);
	VkImageView createImageView(VkImage image, VkFormat format);
	void createImage(VkFormat format,
									 VkImageTiling tiling,
									 VkImageUsageFlags usage,
									 VkMemoryPropertyFlags properties);
	VkSampler createSampler();
};

class MapRenderer
{
public:
	MapRenderer(VulkanInfo &vulkanInfo, MapView *mapView);
	static const int MAX_NUM_TEXTURES = 256 * 256;

	static const int TILE_SIZE = 32;
	static const uint32_t MAX_VERTICES = 64 * 1024;

	void initResources(VkFormat colorFormat);
	void initSwapChainResources(util::Size vulkanSwapChainImageSize);
	void releaseSwapChainResources();
	void releaseResources();

	void startNextFrame();

	VkDescriptorPool &getDescriptorPool()
	{
		return descriptorPool;
	}

	VkDescriptorSetLayout &getTextureDescriptorSetLayout()
	{
		return textureDescriptorSetLayout;
	}

	inline void setCurrentFrame(int frameIndex)
	{
		_currentFrame = &frames[frameIndex];
	}

	FrameData *currentFrame() const noexcept
	{
		return _currentFrame;
	}

private:
	using ItemPredicate = std::function<bool(const Position, const Item &item)>;

	bool debug = false;
	MapView *mapView;
	VulkanInfo &vulkanInfo;
	std::array<FrameData, 3> frames;

	// All sprites are drawn using this index buffer
	BoundBuffer indexBuffer;
	BoundBuffer vertexBuffer;

	VkFormat colorFormat = VK_FORMAT_UNDEFINED;

	FrameData *_currentFrame = nullptr;

	VkRenderPass renderPass;

	VkDescriptorPool descriptorPool = 0;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline = 0;

	VkDescriptorSetLayout uboDescriptorSetLayout = 0;
	VkDescriptorSetLayout textureDescriptorSetLayout;

	// Vulkan texture resources
	std::vector<uint32_t> activeTextureAtlasIds;
	std::vector<VulkanTexture> vulkanTexturesForAppearances;
	/*
		Resources for general textures such as solid color textures (non-sprites)

		NOTE: Textures from TextureAtlas class should be stored in 'vulkanTexturesForAppearances'.
	*/
	std::unordered_map<const Texture *, VulkanTexture> vulkanTextures;

	util::Size vulkanSwapChainImageSize;
	void createRenderPass();
	void createGraphicsPipeline();
	VkShaderModule createShaderModule(const std::vector<uint8_t> &code);
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSetLayouts();
	void createDescriptorSets();
	void createIndexBuffer();
	void createVertexBuffer();

	void updateUniformBuffer();

	void beginRenderPass();
	void setupFrame();

	VkCommandBuffer beginSingleTimeCommands(VulkanInfo *info);
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void drawMap();
	void drawCurrentAction();
	void drawPreviewItem(uint16_t serverId, Position pos);
	void drawMovingSelection();
	void drawRectangle(const Texture &texture, const WorldPosition from, const WorldPosition to, float opacity = 1.0f);
	void drawSolidRectangle(const SolidColor color, const WorldPosition from, const WorldPosition to, float opacity = 1.0f);

	DrawInfo::Object itemDrawInfo(const Item &item, Position position, uint32_t drawFlags);
	DrawInfo::Object itemTypeDrawInfo(const ItemType &itemType, Position position, uint32_t drawFlags);

	VkDescriptorSet objectDescriptorSet(const DrawInfo::Base &info);

	/**
	 * @predicate An Item predicate. Items for which predicate(item) is false will not be rendered.
	 */
	void drawTile(const TileLocation &tileLocation,
								uint32_t drawFlags = ItemDrawFlags::DrawNonSelected,
								Position offset = {},
								const ItemPredicate &filter = {});
	void drawTile(const TileLocation &tileLocation,
								uint32_t drawFlags = ItemDrawFlags::DrawNonSelected,
								const ItemPredicate &filter = {});
	void drawItem(const DrawInfo::Object &info);
	void drawOverlayItemType(uint16_t serverId, const WorldPosition position, const glm::vec4 color = colors::Default);

	void drawRectangle(DrawInfo::Rectangle &info);

	bool shouldDrawItem(const Position pos, const Item &item, uint32_t flags, const ItemPredicate &filter = {}) const noexcept;

	VkDescriptorSet currentDescriptorSet;
};
