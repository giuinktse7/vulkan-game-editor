#pragma once

#include <stdint.h>
#include <ostream>

#include "type_trait.h"
#include "const.h"
#include "util.h"

class MapView;
struct WorldPosition;
struct MapPosition;

template <typename T>
struct BasePosition
{
	BasePosition(T x, T y)
			: x(x), y(y) {}

	T x;
	T y;
};

struct Position : public BasePosition<long>
{
	static std::vector<Position> bresenHams(Position from, Position to);

	Position();
	Position(long x, long y, int z);
	int z;

	void move(long x, long y, int z);

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
	MapPosition mapPos(const MapView &mapView) const;
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
};

struct WorldPosition : public BasePosition<long>
{
	WorldPosition();
	WorldPosition(long x, long y);

	MapPosition mapPos() const;
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

struct MapPosition : public BasePosition<long>
{
	MapPosition();
	MapPosition(long x, long y);

	WorldPosition worldPos() const;

	Position floor(int floor) const;
};

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
inline bool operator==(const Position &pos1, const Position &pos2) noexcept
{
	return pos1.x == pos2.x && pos1.y == pos2.y && pos1.z == pos2.z;
}

inline bool operator!=(const Position &pos1, const Position &pos2) noexcept
{
	return !(pos1 == pos2);
}

inline std::ostream &operator<<(std::ostream &os, const Position &pos)
{
	os << pos.x << ':' << pos.y << ':' << pos.z;
	return os;
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

template <typename T>
inline std::ostream &operator<<(std::ostream &os, const BasePosition<T> &pos)
{
	os << "{ x=" << pos.x << ", y=" << pos.y << " }";
	return os;
}

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>Region2D>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/
template <typename Pos>
struct Region2D
{
	static_assert(is_base_of_template<BasePosition, Pos>::value, "Pos must derive from BasePosition");
	Region2D(Pos from, Pos to)
			: from(from),
				to(to),
				x0(std::min(from.x, to.x)),
				y0(std::min(from.y, to.y)),
				x1(std::max(from.x, to.x)),
				y1(std::max(from.y, to.y)) {}

	Pos from;
	Pos to;

	int x0, y0, x1, y1;

	inline bool contains(Pos pos) const
	{
		return (x0 <= pos.x && pos.x <= x1) && (y0 <= pos.y && pos.y <= y1);
	}
};

namespace std
{
	template <>
	struct tuple_size<Position> : std::integral_constant<size_t, 3>
	{
	};

	template <size_t I>
	class std::tuple_element<I, Position>
	{
	public:
		using type = decltype(declval<Position>().get<I>());
	};

} // namespace std
