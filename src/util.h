#pragma once

#include <type_traits>
#include <variant>
#include <string>
#include <vector>
#include <chrono>
#include <optional>

constexpr bool hasBitSet(uint32_t flag, uint32_t flags)
{
	return (flags & flag) != 0;
}

std::vector<const char *> getRequiredExtensions();

void to_lower_str(std::string &source);

void to_upper_str(std::string &source);

std::string as_lower_str(std::string s);

#pragma warning(push)
#pragma warning(disable : 26812)
template <typename E>
constexpr auto to_underlying(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}
#pragma warning(pop)

namespace util
{
	class Size
	{
	public:
		Size(int width, int height);

		inline constexpr int width() const noexcept;
		inline constexpr int height() const noexcept;

		inline constexpr void setWidth(int width) noexcept;
		inline constexpr void setHeight(int height) noexcept;

	private:
		int w;
		int h;
	};

	template <typename T>
	inline bool contains(std::optional<T> opt, T value)
	{
		return opt.has_value() && opt.value() == value;
	}

	template <typename T>
	inline std::vector<T> sliceLeading(std::vector<T> xs, T x)
	{
		auto cursor = xs.begin();
		while (*cursor == x)
		{
			++cursor;
		}

		return std::vector<T>(cursor, xs.end());
	}

	template <typename T>
	inline void appendVector(std::vector<T> &&source, std::vector<T> &destination)
	{
		if (destination.empty())
			destination = std::move(source);
		else
			destination.insert(std::end(destination),
												 std::make_move_iterator(std::begin(source)),
												 std::make_move_iterator(std::end(source)));
	}

	template <typename T, typename... Args>
	struct is_one_of : std::disjunction<std::is_same<std::decay_t<T>, Args>...>
	{
	};

	// helper type for the visitor #4
	template <class... Ts>
	struct overloaded : Ts...
	{
		using Ts::operator()...;
	};
	// explicit deduction guide (not needed as of C++20)
	template <class... Ts>
	overloaded(Ts...) -> overloaded<Ts...>;

	template <typename T>
	struct Point
	{
		static_assert(std::is_arithmetic<T>::value, "T must be numeric.");
		Point() : _x(0), _y(0) {}
		Point(T x, T y) : _x(x), _y(y) {}

		inline constexpr T x() const noexcept;
		inline constexpr T y() const noexcept;

	private:
		T _x, _y;
	};

	template <typename T>
	struct Rectangle
	{
		static_assert(std::is_arithmetic<T>::value, "T must be numeric.");
		T x1, y1, x2, y2;

		inline Rectangle translate(T dx, T dy) const
		{
			Rectangle rect(*this);
			rect.x1 += dx;
			rect.x2 += dx;

			rect.y1 += dy;
			rect.y2 += dy;

			return rect;
		}

		inline Rectangle translate(T dx, T dy, Point<T> min) const
		{
			Rectangle rect = translate(dx, dy);

			if (rect.x1 < min.x())
			{
				T diff = min.x() - rect.x1;
				rect.x1 += diff;
				rect.x2 += diff;
			}
			if (rect.y1 < min.y())
			{
				T diff = min.y() - rect.y1;
				rect.y1 += diff;
				rect.y2 += diff;
			}

			return rect;
		}
	};

	template <class T>
	inline void combineHash(std::size_t &seed, const T &v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	template <typename T>
	int sgn(T val)
	{
		return (T(0) < val) - (val < T(0));
	}
} // namespace util

inline constexpr int util::Size::width() const noexcept
{
	return w;
}

inline constexpr int util::Size::height() const noexcept
{
	return h;
}

template <typename T>
inline constexpr T util::Point<T>::x() const noexcept
{
	return _x;
}

template <typename T>
inline constexpr T util::Point<T>::y() const noexcept
{
	return _y;
}

inline constexpr void util::Size::setWidth(int width) noexcept
{
	w = width;
}

inline constexpr void util::Size::setHeight(int height) noexcept
{
	h = height;
}

template <typename T>
inline std::ostream &operator<<(std::ostream &os, const util::Point<T> &pos)
{
	os << "{ x=" << pos.x() << ', y=' << pos.y() << ' }';
	return os;
}

template <typename T>
inline std::string toString(T value)
{
	std::ostringstream s;
	s << value;
	return s.str();
}