#pragma once

#include <iterator>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <pfr/core.hpp>
#include <pfr/tuple_size.hpp>

#include <json/scalar_codec.h>

namespace lumalink::json::detail {

inline constexpr size_t pfr_arity_limit = 200;

template <typename Spec>
[[nodiscard]] constexpr expected_void check_min_max_elements(
    const size_t count,
    const context_policy policy,
    const std::string_view failure_message = "element count out of range") noexcept {
    using range_option = typename spec_descriptor<Spec>::min_max_elements_option;

    if constexpr (std::is_void_v<range_option>) {
        (void)count;
        (void)policy;
        (void)failure_message;
        return {};
    } else {
        if (count < range_option::min || count > range_option::max) {
            return std::unexpected(make_error<Spec>(error_code::value_out_of_range, policy, failure_message));
        }

        return {};
    }
}

template <typename T>
struct is_std_optional : std::false_type {};

template <typename Value>
struct is_std_optional<std::optional<Value>> : std::true_type {
    using value_type = Value;
};

template <typename T>
inline constexpr bool is_std_optional_v = is_std_optional<remove_cvref_t<T>>::value;

template <typename Spec>
struct is_optional_spec : std::false_type {};

template <typename ValueSpec>
struct is_optional_spec<spec::optional<ValueSpec>> : std::true_type {};

template <typename Spec>
inline constexpr bool is_optional_spec_v = is_optional_spec<Spec>::value;

template <typename T>
struct is_std_variant : std::false_type {};

template <typename... Values>
struct is_std_variant<std::variant<Values...>> : std::true_type {};

template <typename T>
inline constexpr bool is_std_variant_v = is_std_variant<remove_cvref_t<T>>::value;

template <typename T>
struct variant_size : std::integral_constant<size_t, 0> {};

template <typename... Values>
struct variant_size<std::variant<Values...>> : std::integral_constant<size_t, sizeof...(Values)> {};

template <typename T>
inline constexpr size_t variant_size_v = variant_size<remove_cvref_t<T>>::value;

template <typename T>
struct is_std_tuple : std::false_type {};

template <typename... Values>
struct is_std_tuple<std::tuple<Values...>> : std::true_type {};

template <typename T>
inline constexpr bool is_std_tuple_v = is_std_tuple<remove_cvref_t<T>>::value;

template <typename T, typename = void>
struct is_append_container : std::false_type {};

template <typename T>
struct is_append_container<
    T,
    std::void_t<
        typename remove_cvref_t<T>::value_type,
        decltype(std::declval<remove_cvref_t<T>&>().push_back(std::declval<typename remove_cvref_t<T>::value_type>())),
        decltype(std::declval<const remove_cvref_t<T>&>().size()),
        decltype(std::begin(std::declval<const remove_cvref_t<T>&>())),
        decltype(std::end(std::declval<const remove_cvref_t<T>&>()))>>
    : std::bool_constant<!std::is_same_v<remove_cvref_t<T>, std::string>> {};

template <typename T>
inline constexpr bool is_append_container_v = is_append_container<T>::value;

template <typename T>
using model_type_t = remove_cvref_t<T>;

template <typename Model, size_t FieldCount, typename Enable = void>
struct object_binding_contract {
    static_assert(
        std::is_class_v<model_type_t<Model>>,
        "spec::object auto-binding requires a struct or class model type");
    static_assert(
        std::is_aggregate_v<model_type_t<Model>>,
        "spec::object auto-binding requires an aggregate model type or explicit field trait registration");
};

template <typename Model, size_t FieldCount>
struct object_binding_contract<Model, FieldCount, std::enable_if_t<std::is_union_v<model_type_t<Model>>>> {
    static_assert(
        !std::is_union_v<model_type_t<Model>>,
        "spec::object auto-binding does not support unions; use explicit field trait registration");
};

template <typename Model, size_t FieldCount>
struct object_binding_contract<
    Model,
    FieldCount,
    std::enable_if_t<!std::is_union_v<model_type_t<Model>> && std::is_polymorphic_v<model_type_t<Model>>>> {
    static_assert(
        !std::is_polymorphic_v<model_type_t<Model>>,
        "spec::object auto-binding does not support polymorphic types; use explicit field trait registration");
};

template <typename Model, size_t FieldCount>
struct object_binding_contract<
    Model,
    FieldCount,
    std::enable_if_t<
        !std::is_union_v<model_type_t<Model>> && !std::is_polymorphic_v<model_type_t<Model>> &&
        std::is_aggregate_v<model_type_t<Model>>>> {
    static_assert(FieldCount <= pfr_arity_limit, "object auto-binding exceeds the supported PFR arity limit");

    static constexpr size_t arity = pfr::tuple_size_v<model_type_t<Model>>;

    static_assert(arity <= pfr_arity_limit, "object auto-binding exceeds the supported PFR arity limit");
    static_assert(arity == FieldCount, "object field count must match the bound aggregate arity");
};

template <typename Model, size_t FieldCount>
constexpr void validate_object_binding_target() noexcept {
    (void)sizeof(object_binding_contract<Model, FieldCount>);
}

template <size_t Index, typename T>
using pfr_field_t = remove_cvref_t<decltype(pfr::get<Index>(std::declval<T&>()))>;

template <typename FieldSpec>
[[nodiscard]] constexpr error annotate_field(error value, const context_policy policy) noexcept {
    return with_context(
        value,
        error_context_entry{node_kind::field, logical_name_v<FieldSpec>, FieldSpec::key(), 0, false},
        policy);
}

template <typename FieldSpec>
[[nodiscard]] constexpr error missing_field_error(const context_policy policy) noexcept {
    return with_context(
        error{error_code::missing_field, {}, "missing field"},
        error_context_entry{node_kind::field, logical_name_v<FieldSpec>, FieldSpec::key(), 0, false},
        policy);
}

template <typename Spec>
[[nodiscard]] constexpr error annotate_index(error value, const size_t index, const context_policy policy) noexcept {
    return with_context(
        value,
        error_context_entry{spec_descriptor<Spec>::kind, logical_name_v<Spec>, {}, index, true},
        policy);
}

template <size_t Index, typename Model, typename FieldSpec>
expected_void decode_object_field(const JsonObjectConst source, Model& value, const decode_state& state) {
    using field_value_spec = typename FieldSpec::value_spec;
    using member_type = pfr_field_t<Index, Model>;

    const JsonVariantConst member_source = source[FieldSpec::key_c_str()];
    const bool has_member = !member_source.isUnbound();

    if (!has_member) {
        if constexpr (is_optional_spec_v<field_value_spec>) {
            static_assert(
                is_std_optional_v<member_type>,
                "spec::optional fields require std::optional members on the auto-binding path");
            pfr::get<Index>(value).reset();
            return {};
        } else {
            return std::unexpected(missing_field_error<FieldSpec>(state.context));
        }
    }

    auto decoded = decoder<field_value_spec, member_type>::decode(member_source, state);
    if (!decoded.has_value()) {
        return std::unexpected(annotate_field<FieldSpec>(decoded.error(), state.context));
    }

    pfr::get<Index>(value) = std::move(*decoded);
    return {};
}

template <size_t Index, typename Model>
expected_void decode_object_fields(const JsonObjectConst, Model&, const decode_state&) {
    return {};
}

template <size_t Index, typename Model, typename FirstField, typename... Rest>
expected_void decode_object_fields(const JsonObjectConst source, Model& value, const decode_state& state) {
    auto result = decode_object_field<Index, Model, FirstField>(source, value, state);
    if (!result.has_value()) {
        return result;
    }

    if constexpr (sizeof...(Rest) == 0U) {
        return {};
    } else {
        return decode_object_fields<Index + 1U, Model, Rest...>(source, value, state);
    }
}

template <size_t Index, typename Model, typename FieldSpec>
expected_void encode_object_field(const Model& value, JsonObject destination, const encode_state& state) {
    using field_value_spec = typename FieldSpec::value_spec;
    using member_type = remove_cvref_t<decltype(pfr::get<Index>(value))>;

    const auto& member = pfr::get<Index>(value);
    if constexpr (is_optional_spec_v<field_value_spec>) {
        static_assert(
            is_std_optional_v<member_type>,
            "spec::optional fields require std::optional members on the auto-binding path");
        if (!member.has_value()) {
            return {};
        }
    }

    JsonVariant member_destination = destination[FieldSpec::key_c_str()].template to<JsonVariant>();
    auto encoded = encoder<field_value_spec, member_type>::encode(member, member_destination, state);
    if (!encoded.has_value()) {
        return std::unexpected(annotate_field<FieldSpec>(encoded.error(), state.context));
    }

    return {};
}

template <size_t Index, typename Model>
expected_void encode_object_fields(const Model&, JsonObject, const encode_state&) {
    return {};
}

template <size_t Index, typename Model, typename FirstField, typename... Rest>
expected_void encode_object_fields(const Model& value, JsonObject destination, const encode_state& state) {
    auto result = encode_object_field<Index, Model, FirstField>(value, destination, state);
    if (!result.has_value()) {
        return result;
    }

    if constexpr (sizeof...(Rest) == 0U) {
        return {};
    } else {
        return encode_object_fields<Index + 1U, Model, Rest...>(value, destination, state);
    }
}

template <typename TupleSpec, size_t Index, typename TupleValue>
expected_void decode_tuple_elements(const JsonArrayConst, TupleValue&, const decode_state&) {
    return {};
}

template <typename TupleSpec, size_t Index, typename TupleValue, typename FirstSpec, typename... Rest>
expected_void decode_tuple_elements(const JsonArrayConst source, TupleValue& value, const decode_state& state) {
    using element_type = remove_cvref_t<std::tuple_element_t<Index, remove_cvref_t<TupleValue>>>;
    auto decoded = decoder<FirstSpec, element_type>::decode(source[Index], state);
    if (!decoded.has_value()) {
        return std::unexpected(annotate_index<TupleSpec>(decoded.error(), Index, state.context));
    }

    std::get<Index>(value) = std::move(*decoded);

    if constexpr (sizeof...(Rest) == 0U) {
        return {};
    } else {
        return decode_tuple_elements<TupleSpec, Index + 1U, TupleValue, Rest...>(source, value, state);
    }
}

template <typename TupleSpec, size_t Index, typename TupleValue>
expected_void encode_tuple_elements(const TupleValue&, JsonArray, const encode_state&) {
    return {};
}

template <typename TupleSpec, size_t Index, typename TupleValue, typename FirstSpec, typename... Rest>
expected_void encode_tuple_elements(const TupleValue& value, JsonArray destination, const encode_state& state) {
    JsonVariant element_destination = destination.add<JsonVariant>();
    auto encoded = encoder<FirstSpec, remove_cvref_t<std::tuple_element_t<Index, remove_cvref_t<TupleValue>>>>::encode(
        std::get<Index>(value),
        element_destination,
        state);
    if (!encoded.has_value()) {
        return std::unexpected(annotate_index<TupleSpec>(encoded.error(), Index, state.context));
    }

    if constexpr (sizeof...(Rest) == 0U) {
        return {};
    } else {
        return encode_tuple_elements<TupleSpec, Index + 1U, TupleValue, Rest...>(value, destination, state);
    }
}

template <size_t Index, typename Variant, typename FirstSpec>
expected<Variant> decode_one_of_alternatives(const JsonVariantConst source, const decode_state& state, error* first_error) {
    using alternative_type = std::variant_alternative_t<Index, remove_cvref_t<Variant>>;

    auto decoded = decoder<FirstSpec, alternative_type>::decode(source, state);
    if (decoded.has_value()) {
        return Variant{std::in_place_index<Index>, std::move(*decoded)};
    }

    if (first_error != nullptr && Index == 0U) {
        *first_error = decoded.error();
    }

    return std::unexpected(decoded.error());
}

template <size_t Index, typename Variant, typename FirstSpec, typename SecondSpec, typename... Rest>
expected<Variant> decode_one_of_alternatives(const JsonVariantConst source, const decode_state& state, error* first_error) {
    using alternative_type = std::variant_alternative_t<Index, remove_cvref_t<Variant>>;

    auto decoded = decoder<FirstSpec, alternative_type>::decode(source, state);
    if (decoded.has_value()) {
        return Variant{std::in_place_index<Index>, std::move(*decoded)};
    }

    if (first_error != nullptr && Index == 0U) {
        *first_error = decoded.error();
    }

    return decode_one_of_alternatives<Index + 1U, Variant, SecondSpec, Rest...>(source, state, first_error);
}

template <typename OneOfSpec, size_t Index, typename Variant, typename FirstSpec>
expected_void encode_one_of_alternatives(const Variant& value, JsonVariant destination, const encode_state& state) {
    if (value.index() != Index) {
        return std::unexpected(make_error<OneOfSpec>(error_code::not_implemented, state.context, "variant index mismatch"));
    }

    using alternative_type = std::variant_alternative_t<Index, remove_cvref_t<Variant>>;
    auto encoded = encoder<FirstSpec, alternative_type>::encode(std::get<Index>(value), destination, state);
    if (!encoded.has_value()) {
        return std::unexpected(annotate<OneOfSpec>(encoded.error(), state.context));
    }

    return {};
}

template <typename OneOfSpec, size_t Index, typename Variant, typename FirstSpec, typename SecondSpec, typename... Rest>
expected_void encode_one_of_alternatives(const Variant& value, JsonVariant destination, const encode_state& state) {
    if (value.index() == Index) {
        using alternative_type = std::variant_alternative_t<Index, remove_cvref_t<Variant>>;
        auto encoded = encoder<FirstSpec, alternative_type>::encode(std::get<Index>(value), destination, state);
        if (!encoded.has_value()) {
            return std::unexpected(annotate<OneOfSpec>(encoded.error(), state.context));
        }

        return {};
    }

    return encode_one_of_alternatives<OneOfSpec, Index + 1U, Variant, SecondSpec, Rest...>(
        value,
        destination,
        state);
}

} // namespace lumalink::json::detail

namespace lumalink::json {

template <typename... Fields, typename T>
struct decoder<spec::object<Fields...>, T> {
    static expected<T> decode(const JsonVariantConst source, const decode_state& state) {
        detail::validate_object_binding_target<T, sizeof...(Fields)>();

        if (!source.is<JsonObjectConst>()) {
            return std::unexpected(detail::make_error<spec::object<Fields...>>(
                error_code::unexpected_type,
                state.context,
                "expected object"));
        }

        T value{};
        auto field_result = detail::decode_object_fields<0U, T, Fields...>(source.as<JsonObjectConst>(), value, state);
        if (!field_result.has_value()) {
            return std::unexpected(detail::annotate<spec::object<Fields...>>(field_result.error(), state.context));
        }

        return value;
    }
};

template <typename... Fields, typename T>
struct encoder<spec::object<Fields...>, T> {
    static expected_void encode(const T& value, JsonVariant destination, const encode_state& state) {
        detail::validate_object_binding_target<T, sizeof...(Fields)>();

        JsonObject object_destination = destination.to<JsonObject>();
        auto field_result = detail::encode_object_fields<0U, T, Fields...>(value, object_destination, state);
        if (!field_result.has_value()) {
            return std::unexpected(detail::annotate<spec::object<Fields...>>(field_result.error(), state.context));
        }

        return {};
    }
};

template <typename ValueSpec, typename T>
struct decoder<spec::optional<ValueSpec>, std::optional<T>> {
    static expected<std::optional<T>> decode(const JsonVariantConst source, const decode_state& state) {
        if (source.isNull()) {
            return std::optional<T>{};
        }

        auto decoded = decoder<ValueSpec, T>::decode(source, state);
        if (!decoded.has_value()) {
            return std::unexpected(detail::annotate<spec::optional<ValueSpec>>(decoded.error(), state.context));
        }

        return std::optional<T>{std::move(*decoded)};
    }
};

template <typename ValueSpec, typename T>
struct encoder<spec::optional<ValueSpec>, std::optional<T>> {
    static expected_void encode(const std::optional<T>& value, JsonVariant destination, const encode_state& state) {
        if (!value.has_value()) {
            destination.set(nullptr);
            return {};
        }

        auto encoded = encoder<ValueSpec, T>::encode(*value, destination, state);
        if (!encoded.has_value()) {
            return std::unexpected(detail::annotate<spec::optional<ValueSpec>>(encoded.error(), state.context));
        }

        return {};
    }
};

template <typename ElementSpec, typename... Options, typename T>
struct decoder<spec::array_of<ElementSpec, Options...>, T, std::enable_if_t<detail::is_append_container_v<T>>> {
    static expected<T> decode(const JsonVariantConst source, const decode_state& state) {
        if (!source.is<JsonArrayConst>()) {
            return std::unexpected(detail::make_error<spec::array_of<ElementSpec, Options...>>(
                error_code::unexpected_type,
                state.context,
                "expected array"));
        }

        const JsonArrayConst array_source = source.as<JsonArrayConst>();
        const auto bounds_result = detail::check_min_max_elements<spec::array_of<ElementSpec, Options...>>(
            array_source.size(),
            state.context);
        if (!bounds_result.has_value()) {
            return std::unexpected(bounds_result.error());
        }

        T values{};
        using element_type = typename detail::remove_cvref_t<T>::value_type;

        size_t index = 0U;
        for (const JsonVariantConst element_source : array_source) {
            auto decoded = decoder<ElementSpec, element_type>::decode(element_source, state);
            if (!decoded.has_value()) {
                const auto indexed_error = detail::annotate_index<spec::array_of<ElementSpec, Options...>>(
                    decoded.error(),
                    index,
                    state.context);
                return std::unexpected(detail::annotate<spec::array_of<ElementSpec, Options...>>(
                    indexed_error,
                    state.context));
            }

            values.push_back(std::move(*decoded));
            ++index;
        }

        return values;
    }
};

template <typename ElementSpec, typename... Options, typename T>
struct encoder<spec::array_of<ElementSpec, Options...>, T, std::enable_if_t<detail::is_append_container_v<T>>> {
    static expected_void encode(const T& value, JsonVariant destination, const encode_state& state) {
        const auto bounds_result = detail::check_min_max_elements<spec::array_of<ElementSpec, Options...>>(
            value.size(),
            state.context);
        if (!bounds_result.has_value()) {
            return std::unexpected(bounds_result.error());
        }

        JsonArray array_destination = destination.to<JsonArray>();
        size_t index = 0U;
        for (const auto& element : value) {
            JsonVariant element_destination = array_destination.add<JsonVariant>();
            auto encoded = encoder<ElementSpec, detail::remove_cvref_t<decltype(element)>>::encode(
                element,
                element_destination,
                state);
            if (!encoded.has_value()) {
                const auto indexed_error = detail::annotate_index<spec::array_of<ElementSpec, Options...>>(
                    encoded.error(),
                    index,
                    state.context);
                return std::unexpected(detail::annotate<spec::array_of<ElementSpec, Options...>>(
                    indexed_error,
                    state.context));
            }

            ++index;
        }

        return {};
    }
};

template <typename... Specs, typename T>
struct decoder<spec::tuple<Specs...>, T, std::enable_if_t<detail::is_std_tuple_v<T>>> {
    static expected<T> decode(const JsonVariantConst source, const decode_state& state) {
        static_assert(
            std::tuple_size_v<detail::remove_cvref_t<T>> == sizeof...(Specs),
            "spec::tuple arity must match std::tuple arity");

        if (!source.is<JsonArrayConst>()) {
            return std::unexpected(detail::make_error<spec::tuple<Specs...>>(
                error_code::unexpected_type,
                state.context,
                "expected array"));
        }

        const JsonArrayConst array_source = source.as<JsonArrayConst>();
        if (array_source.size() != sizeof...(Specs)) {
            return std::unexpected(detail::make_error<spec::tuple<Specs...>>(
                error_code::array_size_mismatch,
                state.context,
                "tuple size mismatch"));
        }

        T value{};
        auto decode_result = detail::decode_tuple_elements<spec::tuple<Specs...>, 0U, T, Specs...>(array_source, value, state);
        if (!decode_result.has_value()) {
            return std::unexpected(detail::annotate<spec::tuple<Specs...>>(decode_result.error(), state.context));
        }

        return value;
    }
};

template <typename... Specs, typename T>
struct encoder<spec::tuple<Specs...>, T, std::enable_if_t<detail::is_std_tuple_v<T>>> {
    static expected_void encode(const T& value, JsonVariant destination, const encode_state& state) {
        static_assert(
            std::tuple_size_v<detail::remove_cvref_t<T>> == sizeof...(Specs),
            "spec::tuple arity must match std::tuple arity");

        JsonArray array_destination = destination.to<JsonArray>();
        auto encode_result = detail::encode_tuple_elements<spec::tuple<Specs...>, 0U, T, Specs...>(
            value,
            array_destination,
            state);
        if (!encode_result.has_value()) {
            return std::unexpected(detail::annotate<spec::tuple<Specs...>>(encode_result.error(), state.context));
        }

        return {};
    }
};

template <typename... Specs, typename T>
struct decoder<spec::one_of<Specs...>, T, std::enable_if_t<detail::is_std_variant_v<T> && detail::variant_size_v<T> == sizeof...(Specs)>> {
    static expected<T> decode(const JsonVariantConst source, const decode_state& state) {
        error first_error{};
        auto decoded = detail::decode_one_of_alternatives<0U, T, Specs...>(source, state, &first_error);
        if (!decoded.has_value()) {
            return std::unexpected(detail::annotate<spec::one_of<Specs...>>(first_error, state.context));
        }

        return decoded;
    }
};

template <typename... Specs, typename T>
struct encoder<spec::one_of<Specs...>, T, std::enable_if_t<detail::is_std_variant_v<T> && detail::variant_size_v<T> == sizeof...(Specs)>> {
    static expected_void encode(const T& value, JsonVariant destination, const encode_state& state) {
        return detail::encode_one_of_alternatives<spec::one_of<Specs...>, 0U, T, Specs...>(value, destination, state);
    }
};

} // namespace lumalink::json