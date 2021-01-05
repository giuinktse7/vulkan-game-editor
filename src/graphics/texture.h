#pragma once

#include <filesystem>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#include <memory>

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

    glm::vec4 asVec4() const noexcept;
};

class Texture
{
  public:
    Texture(const std::string &filename);
    Texture(uint32_t width, uint32_t height, uint8_t *pixels);
    Texture(uint32_t width, uint32_t height, std::vector<uint8_t> &&pixels);

    TextureWindow getTextureWindow() const noexcept;
    inline int width() const noexcept;
    inline int height() const noexcept;

    std::vector<uint8_t> copyPixels() const;
    inline const std::vector<uint8_t> &pixels() const noexcept
    {
        return _pixels;
    }

    inline uint32_t sizeInBytes() const;

    static Texture *getSolidTexture(SolidColor color);
    static Texture &getOrCreateSolidTexture(SolidColor color);

  private:
    std::vector<uint8_t> _pixels;

    uint32_t _width;
    uint32_t _height;
};

inline glm::vec4 TextureWindow::asVec4() const noexcept
{
    return glm::vec4(x0, y0, x1, y1);
}

inline int Texture::width() const noexcept
{
    return _width;
}

inline int Texture::height() const noexcept
{
    return _height;
}

inline uint32_t Texture::sizeInBytes() const
{
    return _width * _height * 4;
}

inline std::ostream &operator<<(std::ostream &os, const TextureWindow &info)
{
    os << "{ x0: " << info.x0 << ", y0: " << info.y0 << ", x1: " << info.x1 << ", y1: " << info.y1;
    return os;
}