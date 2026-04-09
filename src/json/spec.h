#pragma once

#include <json/options.h>

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

template <typename Enum, typename... Options>
struct enum_string : detail::scalar_option_contract<Options...> {
    using enum_type = Enum;
};

template <typename... Options>
struct any : detail::scalar_option_contract<Options...> {};

template <typename InnerSpec, typename Codec>
struct with_codec {
    using inner_spec = InnerSpec;
    using codec_type = Codec;
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
struct spec_descriptor;

template <typename... Options>
struct spec_descriptor<spec::null<Options...>> {
    static constexpr node_kind kind = node_kind::null_value;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::boolean<Options...>> {
    static constexpr node_kind kind = node_kind::boolean;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::integer<Options...>> {
    static constexpr node_kind kind = node_kind::integer;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::number<Options...>> {
    static constexpr node_kind kind = node_kind::number;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::string<Options...>> {
    static constexpr node_kind kind = node_kind::string;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename Enum, typename... Options>
struct spec_descriptor<spec::enum_string<Enum, Options...>> {
    static constexpr node_kind kind = node_kind::enum_string;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::any<Options...>> {
    static constexpr node_kind kind = node_kind::any;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using min_max_elements_option = void;
    using pattern_option = typename first_pattern_option<Options...>::type;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename InnerSpec, typename Codec>
struct spec_descriptor<spec::with_codec<InnerSpec, Codec>> {
    static constexpr node_kind kind = spec_descriptor<InnerSpec>::kind;
    using name_option = typename spec_descriptor<InnerSpec>::name_option;
    using min_max_value_option = typename spec_descriptor<InnerSpec>::min_max_value_option;
    using min_max_elements_option = typename spec_descriptor<InnerSpec>::min_max_elements_option;
    using pattern_option = typename spec_descriptor<InnerSpec>::pattern_option;
    using validator_option = typename spec_descriptor<InnerSpec>::validator_option;
};

template <fixed_string Key, typename ValueSpec, typename... Options>
struct spec_descriptor<spec::field<Key, ValueSpec, Options...>> {
    static constexpr node_kind kind = node_kind::field;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using validator_option = void;
};

template <typename... Fields>
struct spec_descriptor<spec::object<Fields...>> {
    static constexpr node_kind kind = node_kind::object;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using validator_option = void;
};

template <typename ElementSpec, typename... Options>
struct spec_descriptor<spec::array_of<ElementSpec, Options...>> {
    static constexpr node_kind kind = node_kind::array_of;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = void;
    using min_max_elements_option = typename first_min_max_elements_option<Options...>::type;
    using pattern_option = void;
    using validator_option = typename first_validator_option<Options...>::type;
};

template <typename... Specs>
struct spec_descriptor<spec::tuple<Specs...>> {
    static constexpr node_kind kind = node_kind::tuple;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using validator_option = void;
};

template <typename ValueSpec>
struct spec_descriptor<spec::optional<ValueSpec>> {
    static constexpr node_kind kind = node_kind::optional;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using validator_option = void;
};

template <typename... Specs>
struct spec_descriptor<spec::one_of<Specs...>> {
    static constexpr node_kind kind = node_kind::one_of;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using validator_option = void;
};

template <>
struct spec_descriptor<spec::error_context_entry> {
    static constexpr node_kind kind = node_kind::object;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using validator_option = void;
};

template <>
struct spec_descriptor<spec::error_context> {
    static constexpr node_kind kind = node_kind::array_of;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using validator_option = void;
};

template <>
struct spec_descriptor<spec::error> {
    static constexpr node_kind kind = node_kind::object;
    using name_option = void;
    using min_max_value_option = void;
    using min_max_elements_option = void;
    using pattern_option = void;
    using validator_option = void;
};

} // namespace lumalink::json::detail
