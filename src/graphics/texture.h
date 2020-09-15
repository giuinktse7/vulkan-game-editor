#pragma once

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
  Texture(const std::string &filename);
  Texture(uint32_t width, uint32_t height, uint8_t *pixels);
  Texture(uint32_t width, uint32_t height, std::vector<uint8_t> &&pixels);

  TextureWindow getTextureWindow();
  inline int getWidth() const;
  inline int getHeight() const;

  std::vector<uint8_t> copyPixels() const;
  inline const std::vector<uint8_t> &pixels() const
  {
    return _pixels;
  }

  static Texture *getSolidTexture(SolidColor color);
  static Texture &getOrCreateSolidTexture(SolidColor color);

private:
  std::vector<uint8_t> _pixels;

  uint32_t width;
  uint32_t height;

  uint32_t mipLevels = 1;

  void init(uint8_t *pixels);
  void init(std::vector<uint8_t> &&pixels);
};

inline int Texture::getWidth() const
{
  return width;
}
inline int Texture::getHeight() const
{
  return height;
}