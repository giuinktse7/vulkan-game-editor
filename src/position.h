#pragma once

#include <ostream>
#include <stdint.h>

#include "const.h"
#include "type_trait.h"
#include "util.h"

class MapView;
struct WorldPosition;

enum GameDirection : uint8_t
{
    DIRECTION_NORTH = 0,
    DIRECTION_EAST = 1,
    DIRECTION_SOUTH = 2,
    DIRECTION_WEST = 3,

    DIRECTION_DIAGONAL_MASK = 4,
    DIRECTION_SOUTHWEST = DIRECTION_DIAGONAL_MASK | 0,
    DIRECTION_SOUTHEAST = DIRECTION_DIAGONAL_MASK | 1,
    DIRECTION_NORTHWEST = DIRECTION_DIAGONAL_MASK | 2,
    DIRECTION_NORTHEAST = DIRECTION_DIAGONAL_MASK | 3,

    DIRECTION_LAST = DIRECTION_NORTHEAST,
    DIRECTION_NONE = 8,
};

template <typename T>
struct BasePosition
{
    BasePosition(T x, T y)
        : x(x), y(y) {}

    T x;
    T y;
};

struct Position
{
    using value_type = int32_t;
    using z_type = int8_t;
    static std::vector<Position> bresenHams(Position from, Position to);
    static std::vector<Position> bresenHamsWithCorners(Position from, Position to);
    static uint32_t tilesInRegion(const Position &from, const Position &to);

    Position();
    Position(value_type x, value_type y, z_type z);

    value_type x;
    value_type y;
    z_type z;

    void move(value_type x, value_type y, z_type z);

    WorldPosition worldPos() const noexcept;

    Position &operator+=(const Position &rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        this->z += rhs.z;
        return *this;
    }

    Position &operator-=(const Position &rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        this->z -= rhs.z;
        return *this;
    }

    friend Position operator-(const Position &lhs, const Position rhs)
    {
        Position pos(lhs);
        pos -= rhs;
        return pos;
    }

    friend Position operator+(const Position &lhs, const Position rhs)
    {
        Position pos(lhs);
        pos += rhs;
        return pos;
    }

    template <int I>
    auto get() const
    {
        if constexpr (I == 0)
            return x;
        else if constexpr (I == 1)
            return y;
        else if constexpr (I == 2)
            return z;
    }
};
STRUCTURED_BINDING(Position, 3);

struct PositionHash
{
    std::size_t operator()(const Position &pos) const noexcept
    {
        size_t hash = 0;
        util::combineHash(hash, pos.x);
        util::combineHash(hash, pos.y);
        util::combineHash(hash, pos.z);

        return hash;
    }
};

struct ScreenPosition : public BasePosition<int>
{
    ScreenPosition();
    ScreenPosition(int x, int y);
    WorldPosition worldPos(const MapView &mapView) const;
    Position toPos(const MapView &mapView) const;

    ScreenPosition &operator-=(const ScreenPosition &rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        return *this;
    }

    friend ScreenPosition operator-(const ScreenPosition &lhs, const ScreenPosition rhs)
    {
        ScreenPosition pos(lhs);
        pos -= rhs;
        return pos;
    }

    ScreenPosition operator*(float value)
    {
        return ScreenPosition(static_cast<int>(std::round(x * value)),
                              static_cast<int>(std::round(y * value)));
    }
};

struct WorldPosition : public BasePosition<int32_t>
{
    using value_type = int32_t;
    WorldPosition();
    WorldPosition(value_type x, value_type y);

    Position toPos(const MapView &mapView) const;
    Position toPos(int floor) const;

    WorldPosition &operator+=(const WorldPosition &rhs)
    {
        this->x += rhs.x;
        this->y += rhs.y;
        return *this;
    }

    friend WorldPosition operator+(const WorldPosition &lhs, const WorldPosition rhs)
    {
        WorldPosition pos(lhs);
        pos += rhs;
        return pos;
    }

    WorldPosition &operator-=(const WorldPosition &rhs)
    {
        this->x -= rhs.x;
        this->y -= rhs.y;
        return *this;
    }

    friend WorldPosition operator-(const WorldPosition &lhs, const WorldPosition rhs)
    {
        WorldPosition pos(lhs);
        pos -= rhs;
        return pos;
    }
};

inline bool operator==(const Position &pos1, const Position &pos2) noexcept
{
    return pos1.x == pos2.x && pos1.y == pos2.y && pos1.z == pos2.z;
}

inline bool operator!=(const Position &pos1, const Position &pos2) noexcept
{
    return !(pos1 == pos2);
}

template <typename T>
inline bool operator==(const BasePosition<T> &pos1, const BasePosition<T> &pos2) noexcept
{
    return pos1.x == pos2.x && pos1.y == pos2.y;
}

template <typename T>
inline bool operator!=(const BasePosition<T> &pos1, const BasePosition<T> &pos2) noexcept
{
    return !(pos1 == pos2);
}

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>Region2D>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/
template <typename Pos>
struct Region2D
{
    static_assert(is_base_of_template<BasePosition, Pos>::value || std::is_same<Position, Pos>::value, "Pos must derive from BasePosition");
    Region2D(Pos from, Pos to)
        : _from(from),
          _to(to),
          _x0(std::min(from.x, to.x)),
          _y0(std::min(from.y, to.y)),
          _x1(std::max(from.x, to.x)),
          _y1(std::max(from.y, to.y)) {}

    inline Pos from() const noexcept
    {
        return _from;
    }

    inline Pos to() const noexcept
    {
        return _to;
    }

    inline void setFrom(Pos position)
    {
        _from = position;
        updateMinMax();
    }

    inline void setTo(Pos position)
    {
        _to = position;
        updateMinMax();
    }

    inline bool contains(Pos pos) const
    {
        return (_x0 <= pos.x && pos.x <= _x1) && (_y0 <= pos.y && pos.y <= _y1);
    }

    inline bool collides(Pos topLeft, Pos bottomRight) const
    {
        return _x0 < bottomRight.x && _x1 > topLeft.x && _y0 < bottomRight.y && _y1 > topLeft.y;
    }

    std::string show() const
    {
        std::ostringstream s;
        s << "Region2D { " << _from << " -> " << _to << " (" << _x0 << ", " << _y0 << ")"
          << " (" << _x1 << ", " << _y1 << ") }";

        return s.str();
    }

    template <size_t I>
    auto &get() &
    {
        if constexpr (I == 0)
            return _from;
        else if constexpr (I == 1)
            return _to;
    }

    template <size_t I>
    auto const &get() const &
    {
        if constexpr (I == 0)
            return _from;
        else if constexpr (I == 1)
            return _to;
    }

    template <size_t I>
    auto &&get() &&
    {
        if constexpr (I == 0)
            return std::move(_from);
        else if constexpr (I == 1)
            return std::move(_to);
    }

  private:
    Pos _from;
    Pos _to;

    int _x0, _y0, _x1, _y1;

    void updateMinMax()
    {
        _x0 = std::min(_from.x, _to.x);
        _y0 = std::min(_from.y, _to.y);
        _x1 = std::max(_from.x, _to.x);
        _y1 = std::max(_from.y, _to.y);
    }
};
STRUCTURED_BINDING_T1(Region2D, 2);

/* Common constants */
namespace PositionConstants
{
    const Position Zero = Position(0, 0, static_cast<int8_t>(0));
}

template <typename T>
inline std::ostream &operator<<(std::ostream &os, const BasePosition<T> &pos)
{
    os << "{ x=" << pos.x << ", y=" << pos.y << " }";
    return os;
}

inline std::ostream &operator<<(std::ostream &os, const Position &pos)
{
    os << pos.x << ':' << pos.y << ':' << static_cast<int>(pos.z);
    return os;
}

template <typename T>
inline std::ostream &operator<<(std::ostream &os, const Region2D<T> &pos)
{
    os << pos.show();
    return os;
}
