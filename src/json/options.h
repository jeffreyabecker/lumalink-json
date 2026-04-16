#pragma once

#include <cstddef>
#include <string>
#include <type_traits>

#include <json/core.h>

namespace lumalink::json::opts {

template <fixed_string Name>
struct name {
    [[nodiscard]] static constexpr std::string_view value() noexcept {
        return Name.view();
    }
};

template <auto Min, auto Max, fixed_string Message = "">
struct min_max_value {
    static constexpr auto min = Min;
    static constexpr auto max = Max;

    [[nodiscard]] static constexpr std::string_view message() noexcept {
        return Message.view();
    }
};

template <auto Min, auto Max, fixed_string Message = "">
struct min_max_elements {
    static_assert(Min <= Max, "opts::min_max_elements requires Min <= Max");

    static constexpr size_t min = static_cast<size_t>(Min);
    static constexpr size_t max = static_cast<size_t>(Max);

    [[nodiscard]] static constexpr std::string_view message() noexcept {
        return Message.view();
    }
};

template <auto Predicate, fixed_string SchemaPattern = "", fixed_string Message = "">
struct pattern {
    static constexpr auto value = Predicate;

    [[nodiscard]] static constexpr std::string_view schema() noexcept {
        return SchemaPattern.view();
    }

    [[nodiscard]] static constexpr std::string_view message() noexcept {
        return Message.view();
    }
};

template <fixed_string Message = "">
struct not_empty {
    [[nodiscard]] static constexpr std::string_view message() noexcept {
        return Message.view();
    }
};

template <auto Validator>
struct validator_func {
    static constexpr auto value = Validator;
};

template <typename Contributor>
struct schema {
    using contributor = Contributor;
};

} // namespace lumalink::json::opts

namespace lumalink::json::schema_meta {

template <fixed_string Key, bool Value>
struct vendor {
    static_assert(Key.view().starts_with("x-"), "schema_meta::vendor keys must start with 'x-'");

    static expected_void apply(JsonVariant schema) {
        JsonObject schema_object = schema.as<JsonObject>();
        schema_object[Key.value] = Value;
        return {};
    }
};

template <fixed_string Key, fixed_string Value>
struct vendor_string {
    static_assert(Key.view().starts_with("x-"), "schema_meta::vendor_string keys must start with 'x-'");

    static expected_void apply(JsonVariant schema) {
        JsonObject schema_object = schema.as<JsonObject>();
        schema_object[Key.value] = std::string(Value.view());
        return {};
    }
};

template <fixed_string Title>
struct title {
    static expected_void apply(JsonVariant schema) {
        JsonObject schema_object = schema.as<JsonObject>();
        schema_object["title"] = std::string(Title.view());
        return {};
    }
};

template <fixed_string Description>
struct description {
    static expected_void apply(JsonVariant schema) {
        JsonObject schema_object = schema.as<JsonObject>();
        schema_object["description"] = std::string(Description.view());
        return {};
    }
};

template <fixed_string Format>
struct format {
    static expected_void apply(JsonVariant schema) {
        JsonObject schema_object = schema.as<JsonObject>();
        schema_object["format"] = std::string(Format.view());
        return {};
    }
};

template <bool Enabled>
struct deprecated {
    static expected_void apply(JsonVariant schema) {
        JsonObject schema_object = schema.as<JsonObject>();
        schema_object["deprecated"] = Enabled;
        return {};
    }
};

template <bool Enabled>
struct read_only {
    static expected_void apply(JsonVariant schema) {
        JsonObject schema_object = schema.as<JsonObject>();
        schema_object["readOnly"] = Enabled;
        return {};
    }
};

template <bool Enabled>
struct write_only {
    static expected_void apply(JsonVariant schema) {
        JsonObject schema_object = schema.as<JsonObject>();
        schema_object["writeOnly"] = Enabled;
        return {};
    }
};

} // namespace lumalink::json::schema_meta

namespace lumalink::json::detail {

template <typename... Types>
struct type_list {};

template <typename...>
inline constexpr bool always_false_v = false;

template <typename Option>
struct is_name_option : std::false_type {};

template <fixed_string Name>
struct is_name_option<opts::name<Name>> : std::true_type {};

template <typename Option>
struct is_min_max_value_option : std::false_type {};

template <auto Min, auto Max, fixed_string Message>
struct is_min_max_value_option<opts::min_max_value<Min, Max, Message>> : std::true_type {};

template <typename Option>
struct is_min_max_elements_option : std::false_type {};

template <auto Min, auto Max, fixed_string Message>
struct is_min_max_elements_option<opts::min_max_elements<Min, Max, Message>> : std::true_type {};

template <typename Option>
struct is_pattern_option : std::false_type {};

template <auto Predicate, fixed_string SchemaPattern, fixed_string Message>
struct is_pattern_option<opts::pattern<Predicate, SchemaPattern, Message>> : std::true_type {};

template <typename Option>
struct is_not_empty_option : std::false_type {};

template <fixed_string Message>
struct is_not_empty_option<opts::not_empty<Message>> : std::true_type {};

template <typename Option>
struct is_validator_option : std::false_type {};

template <auto Validator>
struct is_validator_option<opts::validator_func<Validator>> : std::true_type {};

template <typename Option>
struct is_schema_option : std::false_type {};

template <typename Contributor>
struct is_schema_option<opts::schema<Contributor>> : std::true_type {
    using contributor = Contributor;
};

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
inline constexpr size_t not_empty_option_count_v =
    (0U + ... + static_cast<size_t>(is_not_empty_option<Options>::value));

template <typename... Options>
inline constexpr size_t validator_option_count_v =
    (0U + ... + static_cast<size_t>(is_validator_option<Options>::value));

template <typename List, typename Type>
struct type_list_push_front;

template <typename... Types, typename Type>
struct type_list_push_front<type_list<Types...>, Type> {
    using type = type_list<Type, Types...>;
};

template <bool IsSchemaOption, typename Option, typename Tail>
struct collect_schema_contributors_step {
    using type = Tail;
};

template <typename Option, typename Tail>
struct collect_schema_contributors_step<true, Option, Tail> {
    using type = typename type_list_push_front<Tail, typename is_schema_option<Option>::contributor>::type;
};

template <typename... Options>
struct collect_schema_contributors {
    using type = type_list<>;
};

template <typename First, typename... Rest>
struct collect_schema_contributors<First, Rest...> {
    using tail = typename collect_schema_contributors<Rest...>::type;
    using type = typename collect_schema_contributors_step<is_schema_option<First>::value, First, tail>::type;
};

template <typename... Options>
using schema_contributor_list_t = typename collect_schema_contributors<Options...>::type;

template <typename... Options>
struct scalar_option_contract {
    static_assert(name_option_count_v<Options...> <= 1U, "duplicate opts::name options are not allowed");
    static_assert(
        min_max_value_option_count_v<Options...> <= 1U,
        "duplicate opts::min_max_value options are not allowed");
    static_assert(pattern_option_count_v<Options...> <= 1U, "duplicate opts::pattern options are not allowed");
    static_assert(not_empty_option_count_v<Options...> <= 1U, "duplicate opts::not_empty options are not allowed");
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
    static_assert(not_empty_option_count_v<Options...> <= 1U, "duplicate opts::not_empty options are not allowed");
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
struct first_not_empty_option {
    using type = void;
};

template <typename First, typename... Rest>
struct first_not_empty_option<First, Rest...> {
    using type = std::conditional_t<
        is_not_empty_option<First>::value,
        First,
        typename first_not_empty_option<Rest...>::type>;
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
