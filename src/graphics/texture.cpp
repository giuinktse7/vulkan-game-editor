#include "texture.h"

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

#include <cmath>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <ostream>
#include <unordered_map>

#include "buffer.h"
#include "../util.h"
#include "../logger.h"

#pragma warning(push)
#pragma warning(disable : 26812)
// QVulkanWindow works correctly with UNORM. If not using Vulkan, VK_FORMAT_B8G8R8A8_SRGB works.
constexpr VkFormat ColorFormat = VK_FORMAT_B8G8R8A8_UNORM;
#pragma warning(pop)

std::unordered_map<SolidColor, std::unique_ptr<Texture>> solidColorTextures;

Texture::Texture(uint32_t width, uint32_t height, std::vector<uint8_t> &&pixels)
    : _width(width), _height(height)
{
  init(std::move(pixels));
}

Texture::Texture(uint32_t width, uint32_t height, uint8_t *pixels)
    : _width(width), _height(height)
{
  init(pixels);
}

Texture::Texture(const std::string &filename)
{
  int width, height, channels;

  stbi_uc *pixels = stbi_load(filename.c_str(),
                              &width,
                              &height,
                              &channels,
                              STBI_rgb_alpha);

  // mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

  if (!pixels)
  {
    throw std::runtime_error("failed to load texture image!");
  }

  _width = static_cast<uint32_t>(width);
  _height = static_cast<uint32_t>(height);

  init(pixels);

  stbi_image_free(pixels);
}

void Texture::init(std::vector<uint8_t> &&pixels)
{
  uint64_t sizeInBytes = (static_cast<uint64_t>(_width) * _height) * 4;

  this->_pixels = std::move(pixels);
}

void Texture::init(uint8_t *pixels)
{
  uint64_t sizeInBytes = (static_cast<uint64_t>(_width) * _height) * 4;

  this->_pixels = std::vector<uint8_t>(pixels, pixels + sizeInBytes);
}

TextureWindow Texture::getTextureWindow()
{
  return TextureWindow{0.0f, 0.0f, 1.0f, 1.0f};
}

uint32_t asArgb(SolidColor color)
{
  return to_underlying(color);
}

Texture &Texture::getOrCreateSolidTexture(SolidColor color)
{
  Texture *texture = getSolidTexture(color);
  if (texture)
  {
    return *texture;
  }
  else
  {
    std::vector<uint32_t> buffer(32 * 32);
    std::fill(buffer.begin(), buffer.end(), asArgb(color));

    solidColorTextures.emplace(color, std::make_unique<Texture>(32, 32, (uint8_t *)buffer.data()));

    return *solidColorTextures.at(color).get();
  }
}

Texture *Texture::getSolidTexture(SolidColor color)
{
  auto found = solidColorTextures.find(color);
  return found != solidColorTextures.end() ? found->second.get() : nullptr;
}

std::vector<uint8_t> Texture::copyPixels() const
{
  std::vector<uint8_t> copy = _pixels;
  return copy;
}