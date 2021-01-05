#pragma once

#include <vector>

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>TypeList>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/
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

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
>>>>>>>>>>is_base_of_template>>>>>>>>>
>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
*/
template <template <typename...> class base, typename derived>
struct is_base_of_template_impl
{
    template <typename... Ts>
    static constexpr std::true_type test(const base<Ts...> *);
    static constexpr std::false_type test(...);
    using type = decltype(test(std::declval<derived *>()));
};

template <template <typename...> class base, typename derived>
using is_base_of_template = typename is_base_of_template_impl<base, derived>::type;