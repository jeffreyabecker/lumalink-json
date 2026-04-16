#pragma once

#include <array>

#include <json/options.h>
#include <json/traits.h>

namespace lumalink::json::spec {

template <typename... Options>
struct null : detail::scalar_option_contract<Options...> {};

template <typename... Options>
struct boolean : detail::scalar_option_contract<Options...> {};

template <typename... Options>
struct integer : detail::scalar_option_contract<Options...> {};

template <typename... Options>
struct number : detail::scalar_option_contract<Options...> {};

template <typename... Options>
struct string : detail::scalar_option_contract<Options...> {};

template <auto EnumValue, fixed_string Token, typename... Contributors>
struct enum_value {
    using enum_type = decltype(EnumValue);

    static_assert(std::is_enum_v<enum_type>, "spec::enum_value requires an enum constant");

    static constexpr enum_type value = EnumValue;

    [[nodiscard]] static constexpr std::string_view token() noexcept {
        return Token.view();
    }
};

template <typename... Values>
struct enum_values {
    static_assert(sizeof...(Values) > 0U, "spec::enum_values requires at least one enum_value entry");
};

template <typename Enum, typename Values, typename... Options>
struct enum_string : detail::scalar_option_contract<Options...> {
    static_assert(std::is_enum_v<Enum>, "spec::enum_string requires an enum model type");

    using enum_type = Enum;
    using value_list = Values;
};

template <typename... Options>
struct any : detail::scalar_option_contract<Options...> {};

template <typename InnerSpec, typename Codec>
struct with_codec {
    using inner_spec = InnerSpec;
    using codec_type = Codec;
};

template <typename InnerSpec, typename Binding>
struct with_object_binding {
    using inner_spec = InnerSpec;
    using binding_type = Binding;
};

template <fixed_string Key, typename ValueSpec, typename... Options>
struct field : detail::field_option_contract<Options...> {
    using value_spec = ValueSpec;

    [[nodiscard]] static constexpr std::string_view key() noexcept {
        return Key.view();
    }

    [[nodiscard]] static constexpr const char* key_c_str() noexcept {
        return Key.value;
    }
};

template <typename... Fields>
struct object {};

template <typename ElementSpec, typename... Options>
struct array_of : detail::composite_option_contract<Options...> {
    using element_spec = ElementSpec;
};

template <typename... Specs>
struct tuple {
    static_assert(sizeof...(Specs) > 0U, "spec::tuple requires at least one child spec");
};

template <typename ValueSpec>
struct optional {
    using value_spec = ValueSpec;
};

template <typename... Specs>
struct one_of {
    static_assert(sizeof...(Specs) > 0U, "spec::one_of requires at least one child spec");
};

struct error_context_entry {};

struct error_context {};

struct error {};

} // namespace lumalink::json::spec

namespace lumalink::json::detail {

template <typename Spec>
struct is_enum_value_spec : std::false_type {};

template <auto EnumValue, fixed_string Token, typename... Contributors>
struct is_enum_value_spec<spec::enum_value<EnumValue, Token, Contributors...>> : std::true_type {};

template <typename Spec>
inline constexpr bool is_enum_value_spec_v = is_enum_value_spec<Spec>::value;

template <typename Spec>
struct enum_value_spec_traits;

template <auto EnumValue, fixed_string Token, typename... Contributors>
struct enum_value_spec_traits<spec::enum_value<EnumValue, Token, Contributors...>> {
    using enum_type = decltype(EnumValue);
    using schema_contributors = type_list<Contributors...>;

    static constexpr enum_type value = EnumValue;
    static constexpr bool has_schema_contributors = sizeof...(Contributors) > 0U;

    [[nodiscard]] static constexpr std::string_view token() noexcept {
        return Token.view();
    }
};

template <typename Values>
struct is_enum_value_list_spec : std::false_type {};

template <typename... ValueSpecs>
struct is_enum_value_list_spec<spec::enum_values<ValueSpecs...>> : std::bool_constant<(is_enum_value_spec_v<ValueSpecs> && ...)> {};

template <typename Values>
inline constexpr bool is_enum_value_list_spec_v = is_enum_value_list_spec<Values>::value;

template <typename Enum>
struct enum_string_mapping_entry {
    std::string_view token{};
    Enum value{};
};

template <typename Values>
struct enum_value_list_traits;

template <typename FirstValue, typename... RestValues>
struct enum_value_list_traits<spec::enum_values<FirstValue, RestValues...>> {
    using enum_type = typename enum_value_spec_traits<FirstValue>::enum_type;
    using value_specs = type_list<FirstValue, RestValues...>;

    static constexpr bool has_schema_contributors =
        enum_value_spec_traits<FirstValue>::has_schema_contributors ||
        (... || enum_value_spec_traits<RestValues>::has_schema_contributors);

    [[nodiscard]] static constexpr std::array<enum_string_mapping_entry<enum_type>, 1U + sizeof...(RestValues)>
    runtime_entries() noexcept {
        return {{
            {enum_value_spec_traits<FirstValue>::token(), enum_value_spec_traits<FirstValue>::value},
            {enum_value_spec_traits<RestValues>::token(), enum_value_spec_traits<RestValues>::value}...
        }};
    }
};

template <typename Enum, typename Values, typename = void>
struct enum_value_list_matches_enum : std::false_type {};

template <typename Enum, typename Values>
struct enum_value_list_matches_enum<Enum, Values, std::void_t<typename enum_value_list_traits<Values>::enum_type>>
    : std::bool_constant<std::is_same_v<Enum, typename enum_value_list_traits<Values>::enum_type>> {};

template <typename Enum, typename Values>
inline constexpr bool enum_value_list_matches_enum_v = enum_value_list_matches_enum<Enum, Values>::value;

template <typename Spec, typename Binding>
struct apply_object_binding {
    using type = Spec;
};

template <fixed_string Key, typename ValueSpec, typename... Options, typename Binding>
struct apply_object_binding<spec::field<Key, ValueSpec, Options...>, Binding> {
    using type = spec::field<Key, typename apply_object_binding<ValueSpec, Binding>::type, Options...>;
};

template <typename... Fields, typename Binding>
struct apply_object_binding<spec::object<Fields...>, Binding> {
    using type = spec::with_object_binding<spec::object<typename apply_object_binding<Fields, Binding>::type...>, Binding>;
};

template <typename ElementSpec, typename... Options, typename Binding>
struct apply_object_binding<spec::array_of<ElementSpec, Options...>, Binding> {
    using type = spec::array_of<typename apply_object_binding<ElementSpec, Binding>::type, Options...>;
};

template <typename... Specs, typename Binding>
struct apply_object_binding<spec::tuple<Specs...>, Binding> {
    using type = spec::tuple<typename apply_object_binding<Specs, Binding>::type...>;
};

template <typename ValueSpec, typename Binding>
struct apply_object_binding<spec::optional<ValueSpec>, Binding> {
    using type = spec::optional<typename apply_object_binding<ValueSpec, Binding>::type>;
};

template <typename... Specs, typename Binding>
struct apply_object_binding<spec::one_of<Specs...>, Binding> {
    using type = spec::one_of<typename apply_object_binding<Specs, Binding>::type...>;
};

template <typename InnerSpec, typename Codec, typename Binding>
struct apply_object_binding<spec::with_codec<InnerSpec, Codec>, Binding> {
    using type = spec::with_codec<typename apply_object_binding<InnerSpec, Binding>::type, Codec>;
};

template <typename Spec>
struct spec_descriptor;

template <typename... Options>
struct spec_descriptor<spec::null<Options...>> {
    static constexpr node_kind kind = node_kind::null_value;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using not_empty_option = typename first_not_empty_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::boolean<Options...>> {
    static constexpr node_kind kind = node_kind::boolean;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using not_empty_option = typename first_not_empty_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::integer<Options...>> {
    static constexpr node_kind kind = node_kind::integer;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using not_empty_option = typename first_not_empty_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::number<Options...>> {
    static constexpr node_kind kind = node_kind::number;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using not_empty_option = typename first_not_empty_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::string<Options...>> {
    static constexpr node_kind kind = node_kind::string;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using not_empty_option = typename first_not_empty_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename Enum, typename Values, typename... Options>
struct spec_descriptor<spec::enum_string<Enum, Values, Options...>> {
    static_assert(is_enum_value_list_spec_v<Values>, "spec::enum_string requires spec::enum_values<...>");
    static_assert(
        enum_value_list_matches_enum_v<Enum, Values>,
        "spec::enum_string enum_values entries must all match the declared enum type");

    static constexpr node_kind kind = node_kind::enum_string;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using not_empty_option = typename first_not_empty_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
    using value_list = Values;
};

template <typename... Options>
struct spec_descriptor<spec::any<Options...>> {
    static constexpr node_kind kind = node_kind::any;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using not_empty_option = typename first_not_empty_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename InnerSpec, typename Codec>
struct spec_descriptor<spec::with_codec<InnerSpec, Codec>> {
    static constexpr node_kind kind = spec_descriptor<InnerSpec>::kind;
    using name_option = typename spec_descriptor<InnerSpec>::name_option;
    using min_max_value_option = typename spec_descriptor<InnerSpec>::min_max_value_option;
    using min_max_elements_option = typename spec_descriptor<InnerSpec>::min_max_elements_option;
    using pattern_option = typename spec_descriptor<InnerSpec>::pattern_option;
    using not_empty_option = typename spec_descriptor<InnerSpec>::not_empty_option;
    using validator_option = typename spec_descriptor<InnerSpec>::validator_option;
};

template <typename InnerSpec, typename Binding>
struct spec_descriptor<spec::with_object_binding<InnerSpec, Binding>> {
    static constexpr node_kind kind = spec_descriptor<InnerSpec>::kind;
    using name_option = typename spec_descriptor<InnerSpec>::name_option;
    using min_max_value_option = typename spec_descriptor<InnerSpec>::min_max_value_option;
    using min_max_elements_option = typename spec_descriptor<InnerSpec>::min_max_elements_option;
    using pattern_option = typename spec_descriptor<InnerSpec>::pattern_option;
    using not_empty_option = typename spec_descriptor<InnerSpec>::not_empty_option;
    using validator_option = typename spec_descriptor<InnerSpec>::validator_option;
};

template <fixed_string Key, typename ValueSpec, typename... Options>
struct spec_descriptor<spec::field<Key, ValueSpec, Options...>> {
    static constexpr node_kind kind = node_kind::field;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using not_empty_option = void;
    using validator_option = void;
};

template <typename... Fields>
struct spec_descriptor<spec::object<Fields...>> {
    static constexpr node_kind kind = node_kind::object;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using not_empty_option = void;
    using validator_option = void;
};

template <typename ElementSpec, typename... Options>
struct spec_descriptor<spec::array_of<ElementSpec, Options...>> {
    static constexpr node_kind kind = node_kind::array_of;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = void;
    using min_max_elements_option = typename first_min_max_elements_option<Options...>::type;
    using pattern_option = void;
    using not_empty_option = typename first_not_empty_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Specs>
struct spec_descriptor<spec::tuple<Specs...>> {
    static constexpr node_kind kind = node_kind::tuple;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using not_empty_option = void;
    using validator_option = void;
};

template <typename ValueSpec>
struct spec_descriptor<spec::optional<ValueSpec>> {
    static constexpr node_kind kind = node_kind::optional;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using not_empty_option = void;
    using validator_option = void;
};

template <typename... Specs>
struct spec_descriptor<spec::one_of<Specs...>> {
    static constexpr node_kind kind = node_kind::one_of;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using not_empty_option = void;
    using validator_option = void;
};

template <>
struct spec_descriptor<spec::error_context_entry> {
    static constexpr node_kind kind = node_kind::object;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using not_empty_option = void;
    using validator_option = void;
};

template <>
struct spec_descriptor<spec::error_context> {
    static constexpr node_kind kind = node_kind::array_of;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using not_empty_option = void;
    using validator_option = void;
};

template <>
struct spec_descriptor<spec::error> {
    static constexpr node_kind kind = node_kind::object;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using not_empty_option = void;
    using validator_option = void;
};

} // namespace lumalink::json::detail
