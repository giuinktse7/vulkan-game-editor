#include "texture.h"

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(pop)

#include <algorithm>
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "../logger.h"
#include "../util.h"
#include "buffer.h"

#pragma warning(push)
#pragma warning(disable : 26812)
#pragma warning(pop)

std::unordered_map<SolidColor, std::unique_ptr<Texture>> solidColorTextures;

Texture::Texture(uint32_t width, uint32_t height, std::vector<uint8_t> &&pixels)
    : _width(width), _height(height)
{
    this->_pixels = std::move(pixels);
}

Texture::Texture(uint32_t width, uint32_t height, uint8_t *pixels)
    : _width(width), _height(height)
{
    uint64_t sizeInBytes = (static_cast<uint64_t>(_width) * _height) * 4;
    this->_pixels = std::vector<uint8_t>(pixels, pixels + sizeInBytes);
}

Texture::Texture(const std::string &filename)
{
    int width, height, channels;

    stbi_uc *pixels = stbi_load(filename.c_str(),
                                &width,
                                &height,
                                &channels,
                                STBI_rgb_alpha);

    if (!pixels)
    {
        throw std::runtime_error("failed to load texture image!");
    }

    _width = static_cast<uint32_t>(width);
    _height = static_cast<uint32_t>(height);

    uint64_t sizeInBytes = (static_cast<uint64_t>(_width) * _height) * 4;
    this->_pixels = std::vector<uint8_t>(pixels, pixels + sizeInBytes);

    stbi_image_free(pixels);
}

TextureWindow Texture::getTextureWindow() const noexcept
{
    return TextureWindow{0.0f, 0.0f, 1.0f, 1.0f};
}

uint32_t asArgb(SolidColor color) noexcept
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

glm::vec4 TextureWindow::asVec4() const noexcept
{
    return glm::vec4(x0, y0, x1, y1);
}

Pixel getPixelFromBMPTexture(int x, int y, int atlasWidth, const std::vector<uint8_t> &pixels)
{
    int i = (atlasWidth - y) * (atlasWidth * 4) + x * 4;

    // Because the texture is stored in the BMP format, pixels are read in reverse order (as BGRA instead of ARGB)
    Pixel pixel{};
    pixel.parts._b = pixels.at(i);
    pixel.parts._g = pixels.at(i + 1);
    pixel.parts._r = pixels.at(i + 2);
    pixel.parts._a = pixels.at(i + 3);

    return pixel;
}

// See https://stackoverflow.com/questions/45041273/how-to-correctly-multiply-two-colors-with-byte-components#comment77056973_45041273
Pixel Pixel::multiply(const Pixel &other)
{
    return {
        (uint8_t)((r() * other.r() + 0xFF) >> 8),
        (uint8_t)((g() * other.g() + 0xFF) >> 8),
        (uint8_t)((b() * other.b() + 0xFF) >> 8),
        (uint8_t)((a() * other.a() + 0xFF) >> 8)};
}

void multiplyPixelInBMP(int x, int y, int atlasWidth, std::vector<uint8_t> &pixels, const Pixel &pixel)
{
    int i = (atlasWidth - y) * (atlasWidth * 4) + x * 4;

    // Because the texture is stored in the BMP format, pixels are read in reverse order (as BGRA instead of ARGB)
    pixels[i] = (uint8_t)((pixels[i] * pixel.b() + 0xFF) >> 8);
    pixels[i + 1] = (uint8_t)((pixels[i + 1] * pixel.g() + 0xFF) >> 8);
    pixels[i + 2] = (uint8_t)((pixels[i + 2] * pixel.r() + 0xFF) >> 8);
    pixels[i + 3] = (uint8_t)((pixels[i + 3] * pixel.a() + 0xFF) >> 8);
}
