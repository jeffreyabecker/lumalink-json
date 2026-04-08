#pragma once

#include <type_traits>

#include <json/core.h>

namespace lumalink::json::opts {

template <fixed_string Name>
struct name {
    [[nodiscard]] static constexpr std::string_view value() noexcept {
        return Name.view();
    }
};

template <auto Min, auto Max>
struct min_max_value {
    static constexpr auto min = Min;
    static constexpr auto max = Max;
};

template <auto Predicate>
struct pattern {
    static constexpr auto value = Predicate;
};

} // namespace lumalink::json::opts

namespace lumalink::json::detail {

template <typename...>
inline constexpr bool always_false_v = false;

template <typename Option>
struct is_name_option : std::false_type {};

template <fixed_string Name>
struct is_name_option<opts::name<Name>> : std::true_type {};

template <typename Option>
struct is_min_max_value_option : std::false_type {};

template <auto Min, auto Max>
struct is_min_max_value_option<opts::min_max_value<Min, Max>> : std::true_type {};

template <typename Option>
struct is_pattern_option : std::false_type {};

template <auto Predicate>
struct is_pattern_option<opts::pattern<Predicate>> : std::true_type {};

template <typename... Options>
inline constexpr size_t name_option_count_v = (0U + ... + static_cast<size_t>(is_name_option<Options>::value));

template <typename... Options>
inline constexpr size_t min_max_value_option_count_v =
    (0U + ... + static_cast<size_t>(is_min_max_value_option<Options>::value));

template <typename... Options>
inline constexpr size_t pattern_option_count_v =
    (0U + ... + static_cast<size_t>(is_pattern_option<Options>::value));

template <typename... Options>
struct scalar_option_contract {
    static_assert(name_option_count_v<Options...> <= 1U, "duplicate opts::name options are not allowed");
    static_assert(
        min_max_value_option_count_v<Options...> <= 1U,
        "duplicate opts::min_max_value options are not allowed");
    static_assert(pattern_option_count_v<Options...> <= 1U, "duplicate opts::pattern options are not allowed");
};

template <typename... Options>
struct first_name_option {
    using type = void;
};

template <typename First, typename... Rest>
struct first_name_option<First, Rest...> {
    using type = std::conditional_t<
        is_name_option<First>::value,
        First,
        typename first_name_option<Rest...>::type>;
};

template <typename... Options>
struct first_min_max_value_option {
    using type = void;
};

template <typename First, typename... Rest>
struct first_min_max_value_option<First, Rest...> {
    using type = std::conditional_t<
        is_min_max_value_option<First>::value,
        First,
        typename first_min_max_value_option<Rest...>::type>;
};

template <typename... Options>
struct first_pattern_option {
    using type = void;
};

template <typename First, typename... Rest>
struct first_pattern_option<First, Rest...> {
    using type = std::conditional_t<
        is_pattern_option<First>::value,
        First,
        typename first_pattern_option<Rest...>::type>;
};

} // namespace lumalink::json::detail
