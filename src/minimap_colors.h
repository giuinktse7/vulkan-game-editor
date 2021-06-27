#pragma once

#include <sstream>

struct RGBA
{
    int r;
    int g;
    int b;
    int a;
};

struct MinimapColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
    uint32_t rgba;

    constexpr MinimapColor(uint8_t r, uint8_t g, uint8_t b)
        : r(r), g(g), b(b), a(0xFF), rgba(r << 24 | g << 16 | b << 8 | a)
    {
    }

    constexpr RGBA rgbaInfo() const noexcept
    {
        return {r, g, b, a};
    }

    constexpr MinimapColor()
        : r(0), g(0), b(0), a(0xFF), rgba(0x000000FF)
    {
    }
};

struct ConstexprColors
{
    constexpr ConstexprColors()
        : colors()
    {
        for (int i = 0; i < 256; ++i)
        {
            uint8_t r = i / 36 * 51;
            uint8_t g = ((i / 6) % 6) * 51;
            uint8_t b = (i % 6) * 51;

            colors[i] = MinimapColor(r, g, b);
        }
    }

    constexpr const MinimapColor &operator[](std::size_t i) const
    {
        return colors[i];
    }

    MinimapColor colors[256];
};

class MinimapColors
{
  public:
    static constexpr ConstexprColors colors{};

  private:
};

inline std::ostream &operator<<(std::ostream &os, const MinimapColor &color)
{
    os << "(" << color.r << ", " << color.g << ", " << color.b << ", " << color.a << ")";
    return os;
}