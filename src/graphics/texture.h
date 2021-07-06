#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <glm/vec4.hpp>
#include <memory>

class TextureAtlas;
struct Pixel;

enum class SolidColor : uint32_t
{
    Black = 0xFF000000,
    Blue = 0xFF039BE5,
    Red = 0xFFFF0000,
    Green = 0xFF00FF00
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

    inline int width() const noexcept;
    inline int height() const noexcept;

    Texture deepCopy() const;

    std::vector<uint8_t> copyPixels() const;
    inline const std::vector<uint8_t> &pixels() const noexcept
    {
        return _pixels;
    }

    inline uint32_t sizeInBytes() const;

    static Texture *getSolidTexture(SolidColor color);
    static Texture &getOrCreateSolidTexture(SolidColor color);

    /*
		An auto-incrementing id (used for vector indexing)
	*/
    uint32_t id() const noexcept
    {
        return _id;
    }

    bool finalized() const noexcept;
    void finalize();

  private:
    friend class TextureAtlas;

    Pixel getPixel(int x, int y) const;
    void multiplyPixel(int x, int y, Pixel pixel);
    static uint32_t nextTextureId();
    static uint32_t _nextTextureId;

    uint32_t _id;
    std::vector<uint8_t> _pixels;

    uint32_t _width;
    uint32_t _height;

    // A texture marked as finalized can no longer be changed.
    bool _finalized = false;
};

struct Pixel
{
    union
    {
        struct
        {
            uint8_t _r;
            uint8_t _g;
            uint8_t _b;
            uint8_t _a;
        } parts;
        uint32_t full;
    };

    uint8_t r() const noexcept
    {
        return parts._r;
    }
    uint8_t g() const noexcept
    {
        return parts._g;
    }
    uint8_t b() const noexcept
    {
        return parts._b;
    }
    uint8_t a() const noexcept
    {
        return parts._a;
    }

    Pixel multiply(const Pixel &other);
};

struct Pixels
{
    static constexpr Pixel Red{255, 0, 0, 255};
    static constexpr Pixel Green{0, 255, 0, 255};
    static constexpr Pixel Blue{0, 0, 255, 255};
    static constexpr Pixel Yellow{255, 255, 0, 255};
    static constexpr Pixel Magenta{255, 0, 255, 0};
};

inline bool operator==(const Pixel &lhs, const Pixel &rhs)
{
    // return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
    return lhs.full == rhs.full;
}

inline bool operator!=(const Pixel &lhs, const Pixel &rhs)
{
    return !(lhs == rhs);
}

Pixel getPixelFromBMPTexture(int x, int y, int atlasWidth, const std::vector<uint8_t> &pixels);
void multiplyPixelInBMP(int x, int y, int atlasWidth, std::vector<uint8_t> &pixels, const Pixel &pixel);

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