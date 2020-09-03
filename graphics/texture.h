#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <filesystem>

#include <memory>

#include "device_manager.h"

enum class SolidColor : uint32_t
{
  Black = 0xFF000000,
  Blue = 0xFF039BE5,
  Blue2 = 0xFF0000FF,
  Red = 0xFFFF0000,
  Green = 0xFF00FF00,
  Test = 0xFFAA0000
};

struct TextureWindow
{
  float x0;
  float y0;
  float x1;
  float y1;
};

class Texture
{
public:
  struct Descriptor
  {
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
  };

  Texture(const std::string &filename, Texture::Descriptor descriptor);
  Texture(uint32_t width, uint32_t height, uint8_t *pixels, Texture::Descriptor descriptor);
  Texture(uint32_t width, uint32_t height, std::vector<uint8_t> pixels, Texture::Descriptor descriptor);

  VkDescriptorSet getDescriptorSet();
  TextureWindow getTextureWindow();
  int getWidth();
  int getHeight();

  VkSampler createSampler();

  static Texture *getSolidTexture(SolidColor color);
  static Texture &getOrCreateSolidTexture(SolidColor color, Texture::Descriptor descriptor);

  void update(Texture::Descriptor descriptor);

  void releaseVulkanResources();
  static void releaseSolidColorTextures();

private:
  bool initialized = false;

  std::vector<uint8_t> pixels;

  uint32_t width;
  uint32_t height;

  VkDeviceSize imageSize;
  VkImage textureImage;
  VkDeviceMemory textureImageMemory;

  VkDescriptorSet descriptorSet;
  VkDevice device;

  // Used to check whether the descriptorSet needs to be recreated
  VkDescriptorSetLayout layout;

  uint32_t mipLevels = 1;

  void init(uint32_t width, uint32_t height, uint8_t *pixels, Texture::Descriptor descriptor);

  void createImage(uint32_t width,
                   uint32_t height,
                   uint32_t mipLevels,
                   VkFormat format,
                   VkImageTiling tiling,
                   VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties,
                   VkImage &image,
                   VkDeviceMemory &imageMemory);

  VkDescriptorSet createDescriptorSet(Texture::Descriptor descriptor);

  void initVulkanResources(Texture::Descriptor descriptor);
};
