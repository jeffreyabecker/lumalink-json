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

} // namespace lumalink::json::spec

namespace lumalink::json::detail {

template <typename Spec>
struct spec_descriptor;

template <typename... Options>
struct spec_descriptor<spec::null<Options...>> {
    static constexpr node_kind kind = node_kind::null_value;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using pattern_option = typename first_pattern_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::boolean<Options...>> {
    static constexpr node_kind kind = node_kind::boolean;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using pattern_option = typename first_pattern_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::integer<Options...>> {
    static constexpr node_kind kind = node_kind::integer;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using pattern_option = typename first_pattern_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::number<Options...>> {
    static constexpr node_kind kind = node_kind::number;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using pattern_option = typename first_pattern_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::string<Options...>> {
    static constexpr node_kind kind = node_kind::string;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using pattern_option = typename first_pattern_option<Options...>::type;
};

template <typename Enum, typename... Options>
struct spec_descriptor<spec::enum_string<Enum, Options...>> {
    static constexpr node_kind kind = node_kind::enum_string;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using pattern_option = typename first_pattern_option<Options...>::type;
};

template <typename... Options>
struct spec_descriptor<spec::any<Options...>> {
    static constexpr node_kind kind = node_kind::any;
    using name_option = typename first_name_option<Options...>::type;
    using min_max_value_option = typename first_min_max_value_option<Options...>::type;
    using pattern_option = typename first_pattern_option<Options...>::type;
};

} // namespace lumalink::json::detail
