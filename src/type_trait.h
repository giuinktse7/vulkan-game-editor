#pragma once

#include <vector>

template <typename... T>
struct TypeList;

template <typename H, typename... T>
struct TypeList<H, T...>
{
	using head = H;
	using tail = TypeList<T...>;

	static inline std::vector<const char *> names()
	{
		std::vector<const char *> result{typeid(head).name()};

		std::vector<const char *> next = tail::names();
		result.insert(std::end(result), std::begin(next), std::end(next));

		return result;
	}

	inline std::vector<const char *> typeNames() const
	{
		std::vector<const char *> result{typeid(head).name()};

		std::vector<const char *> next = tail::names();
		result.insert(std::end(result), std::begin(next), std::end(next));

		return result;
	}
};

template <>
struct TypeList<>
{
	static inline std::vector<const char *> names()
	{
		return std::vector<const char *>{};
	}
	inline std::vector<const char *> typeNames() const
	{
		return std::vector<const char *>{};
	}
};