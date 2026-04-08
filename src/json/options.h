#pragma once

#include <cstddef>
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

template <auto Min, auto Max>
struct min_max_elements {
    static_assert(Min <= Max, "opts::min_max_elements requires Min <= Max");

    static constexpr size_t min = static_cast<size_t>(Min);
    static constexpr size_t max = static_cast<size_t>(Max);
};

template <auto Predicate, fixed_string SchemaPattern = "">
struct pattern {
    static constexpr auto value = Predicate;

    [[nodiscard]] static constexpr std::string_view schema() noexcept {
        return SchemaPattern.view();
    }
};

template <auto Validator>
struct validator_func {
    static constexpr auto value = Validator;
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
struct is_min_max_elements_option : std::false_type {};

template <auto Min, auto Max>
struct is_min_max_elements_option<opts::min_max_elements<Min, Max>> : std::true_type {};

template <typename Option>
struct is_pattern_option : std::false_type {};

template <auto Predicate, fixed_string SchemaPattern>
struct is_pattern_option<opts::pattern<Predicate, SchemaPattern>> : std::true_type {};

template <typename Option>
struct is_validator_option : std::false_type {};

template <auto Validator>
struct is_validator_option<opts::validator_func<Validator>> : std::true_type {};

template <typename... Options>
inline constexpr size_t name_option_count_v = (0U + ... + static_cast<size_t>(is_name_option<Options>::value));

template <typename... Options>
inline constexpr size_t min_max_value_option_count_v =
    (0U + ... + static_cast<size_t>(is_min_max_value_option<Options>::value));

template <typename... Options>
inline constexpr size_t min_max_elements_option_count_v =
    (0U + ... + static_cast<size_t>(is_min_max_elements_option<Options>::value));

template <typename... Options>
inline constexpr size_t pattern_option_count_v =
    (0U + ... + static_cast<size_t>(is_pattern_option<Options>::value));

template <typename... Options>
inline constexpr size_t validator_option_count_v =
    (0U + ... + static_cast<size_t>(is_validator_option<Options>::value));

template <typename... Options>
struct scalar_option_contract {
    static_assert(name_option_count_v<Options...> <= 1U, "duplicate opts::name options are not allowed");
    static_assert(
        min_max_value_option_count_v<Options...> <= 1U,
        "duplicate opts::min_max_value options are not allowed");
    static_assert(pattern_option_count_v<Options...> <= 1U, "duplicate opts::pattern options are not allowed");
    static_assert(
        validator_option_count_v<Options...> <= 1U,
        "duplicate opts::validator_func options are not allowed");
};

template <typename... Options>
struct composite_option_contract {
    static_assert(name_option_count_v<Options...> <= 1U, "duplicate opts::name options are not allowed");
    static_assert(
        min_max_elements_option_count_v<Options...> <= 1U,
        "duplicate opts::min_max_elements options are not allowed");
    static_assert(
        validator_option_count_v<Options...> <= 1U,
        "duplicate opts::validator_func options are not allowed");
};

template <typename... Options>
struct field_option_contract {
    static_assert(name_option_count_v<Options...> <= 1U, "duplicate opts::name options are not allowed");
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

template <typename... Options>
struct first_validator_option {
    using type = void;
};

template <typename First, typename... Rest>
struct first_validator_option<First, Rest...> {
    using type = std::conditional_t<
        is_validator_option<First>::value,
        First,
        typename first_validator_option<Rest...>::type>;
};

template <typename... Options>
struct first_min_max_elements_option {
    using type = void;
};

template <typename First, typename... Rest>
struct first_min_max_elements_option<First, Rest...> {
    using type = std::conditional_t<
        is_min_max_elements_option<First>::value,
        First,
        typename first_min_max_elements_option<Rest...>::type>;
};

} // namespace lumalink::json::detail
