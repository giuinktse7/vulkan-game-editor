#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <unordered_set>
#include <memory>

#include "device_manager.h"
#include "texture.h"
#include "resource-descriptor.h"
#include "buffer.h"
#include "vertex.h"

#include "../camera.h"
#include "../map_view.h"
#include "../random.h"
#include "../util.h"
#include "../time_point.h"
#include "../position.h"
#include "../map_renderer.h"

class Map;

enum class FrameResult
{
	Failure = 0,
	Success = 1
};

class Engine;

extern Engine *g_engine;

namespace engine
{

	void create();

} // namespace engine

class Engine
{
public:
	Engine();
	~Engine();

	static const int TILE_SIZE = 32;
	const glm::vec4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

	TimePoint startTime;

	/*
		Used by animators for synchronous animations
	*/
	TimePoint currentTime;

	uint32_t currentFrameIndex;

	Random random;

	bool debug = false;

	// Can be set to false if the UI is currently capturing the mouse.
	bool captureMouse = true;
	bool captureKeyboard = true;

	struct FrameData
	{
		VkSemaphore imageAvailableSemaphore = nullptr;
		VkSemaphore renderCompleteSemaphore = nullptr;
		VkFence inFlightFence = nullptr;
	};

	VkInstance &getVkInstance()
	{
		return instance;
	}

	void setPhysicalDevice(VkPhysicalDevice physicalDevice)
	{
		this->physicalDevice = physicalDevice;
	}

	VkSurfaceKHR &getSurface()
	{
		return surface;
	}

	bool &getFramebufferResized()
	{
		return framebufferResized;
	}

	SwapChain &getSwapChain()
	{
		return swapChain;
	}

	VkPhysicalDevice &getPhysicalDevice()
	{
		return physicalDevice;
	}

	VkQueue *getGraphicsQueue()
	{
		return &graphicsQueue;
	}

	VkQueue *getPresentQueue()
	{
		return &presentQueue;
	}

	void setFrameBufferResized(bool value)
	{
		framebufferResized = value;
	}

	bool hasFrameBufferResized() const
	{
		return framebufferResized;
	}

	VkDevice &getDevice()
	{
		return device;
	}

	VkCommandPool getCommandPool()
	{
		return commandPool;
	}

	void clearCurrentCommandBuffer()
	{
		currentCommandBuffer = nullptr;
	}

	void createCommandPool();

	void transitionImageLayout(VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer buffer);

	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void shutdown();

	void WaitUntilDeviceIdle();

	QueueFamilyIndices getQueueFamilyIndices()
	{
		return queueFamilyIndices;
	}

	VkDescriptorSetLayout &getPerTextureDescriptorSetLayout()
	{
		return perTextureDescriptorSetLayout;
	}

	void initialize();

	void createSyncObjects();
	void cleanupSyncObjects();

	uint32_t getMaxFramesInFlight();

	bool isValidWindowSize();

	void setCursorPos(ScreenPosition pos)
	{
		this->prevCursorPos = this->cursorPos;
		this->cursorPos = pos;
	}

	ScreenPosition getCursorPos() const
	{
		return cursorPos;
	}

	ScreenPosition getPrevCursorPos() const
	{
		return prevCursorPos;
	}

	VkAllocationCallbacks *getAllocator()
	{
		return allocator;
	}

	uint32_t getImageCount()
	{
		return swapChain.getImageCount();
	}

	uint32_t getMinImageCount()
	{
		return swapChain.getMinImageCount();
	}

	uint32_t getWidth()
	{
		return swapChain.getExtent().width;
	}
	uint32_t getHeight()
	{
		return swapChain.getExtent().height;
	}

	bool initFrame();
	FrameResult nextFrame();

	VkShaderModule createShaderModule(const std::vector<uint8_t> &code);

	void resetZoom()
	{
		mapView->resetZoom();
	}

	void zoomIn()
	{
		mapView->zoomIn();
	}

	void zoomOut()
	{
		mapView->zoomOut();
	}

	void translateCamera(glm::vec3 delta);
	void translateCameraZ(int z);

	// VkDescriptorPool &getMapDescriptorPool()
	// {
	// 	return mapRenderer->getDescriptorPool();
	// }

	// VkDescriptorSetLayout &getTextureDescriptorSetLayout()
	// {
	// 	return mapRenderer->getTextureDescriptorSetLayout();
	// }

	const std::optional<uint16_t> getSelectedServerId() const
	{
		// TODO
		return {};
	}

	bool hasBrush() const
	{
		// TODO
		return false;
	}

	MapView *getMapView() const
	{
		return mapView.get();
	}

private:
	std::array<FrameData, 3> frames;
	// Fences for vkAcquireNextImageKHR
	std::array<VkFence, 3> swapChainImageInFlight;
	FrameData *currentFrame = nullptr;

	ScreenPosition prevCursorPos;
	ScreenPosition cursorPos;
	bool isInitialized = false;

	int width;
	int height;

	VkSurfaceKHR surface;

	VkInstance instance;
	VkAllocationCallbacks *allocator = NULL;

	SwapChain swapChain;

	VkDevice device;
	VkPhysicalDevice physicalDevice;

	QueueFamilyIndices queueFamilyIndices;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	std::unique_ptr<MapView> mapView;
	std::unique_ptr<MapRenderer> mapRenderer;

	VkDebugUtilsMessengerEXT debugMessenger;

	bool framebufferResized = false;

	VkCommandPool commandPool;

	std::vector<BoundBuffer> uniformBuffers;

	VkDescriptorSetLayout perTextureDescriptorSetLayout;
	VkDescriptorSetLayout perFrameDescriptorSetLayout;

	VkCommandBuffer currentCommandBuffer;

	uint32_t previousFrame;

	void createVulkanInstance();

	static void framebufferResizeCallback(int width, int height);

	static bool checkValidationLayerSupport();
	static std::vector<const char *> getRequiredExtensions();
	static bool chronosOrStandardValidation(std::vector<VkLayerProperties> &props);
	void createSurface();

	void setFrameIndex(uint32_t index);

	void setSurface(VkSurfaceKHR &surface)
	{
		this->surface = surface;
	}

	void recreateSwapChain();
};
