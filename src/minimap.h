#pragma once

#include <sstream>

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>Compile-time Minimap colors>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

struct RGBA
{
    int r;
    int g;
    int b;
    int a;
};

struct MinimapColor
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
    uint32_t rgba;

    constexpr MinimapColor(uint8_t r, uint8_t g, uint8_t b)
        : red(r), green(g), blue(b), alpha(0xFF), rgba(r << 24 | g << 16 | b << 8 | alpha)
    {
    }

    constexpr RGBA rgbaInfo() const noexcept
    {
        return {red, green, blue, alpha};
    }

    constexpr MinimapColor()
        : red(0), green(0), blue(0), alpha(0xFF), rgba(0x000000FF)
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

//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>End of Compile-time Minimap colors>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

class Minimap
{
  public:
    static constexpr ConstexprColors colors{};

  private:
};

inline std::ostream &operator<<(std::ostream &os, const MinimapColor &color)
{
    os << "(" << color.red << ", " << color.green << ", " << color.blue << ", " << color.alpha << ")";
    return os;
}