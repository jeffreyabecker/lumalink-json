#pragma once

#include <cstring>
#include <expected>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include <json/core.h>
#include <json/spec.h>
#include <json/traits.h>

namespace lumalink::json::detail {

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename Codec, typename InnerSpec, typename T, typename = void>
struct has_wrapped_codec_decode : std::false_type {};

template <typename Codec, typename InnerSpec, typename T>
struct has_wrapped_codec_decode<
    Codec,
    InnerSpec,
    T,
    std::void_t<decltype(Codec::template decode<InnerSpec, T>(
        std::declval<JsonVariantConst>(),
        std::declval<const decode_state&>()))>> : std::true_type {};

template <typename Codec, typename InnerSpec, typename T>
inline constexpr bool has_wrapped_codec_decode_v = has_wrapped_codec_decode<Codec, InnerSpec, T>::value;

template <typename Codec, typename InnerSpec, typename T, typename = void>
struct has_wrapped_codec_encode : std::false_type {};

template <typename Codec, typename InnerSpec, typename T>
struct has_wrapped_codec_encode<
    Codec,
    InnerSpec,
    T,
    std::void_t<decltype(Codec::template encode<InnerSpec, T>(
        std::declval<const T&>(),
        std::declval<JsonVariant>(),
        std::declval<const encode_state&>()))>> : std::true_type {};

template <typename Codec, typename InnerSpec, typename T>
inline constexpr bool has_wrapped_codec_encode_v = has_wrapped_codec_encode<Codec, InnerSpec, T>::value;

template <typename T>
inline constexpr bool is_supported_string_target_v =
    std::is_same_v<remove_cvref_t<T>, std::string> ||
    std::is_same_v<remove_cvref_t<T>, std::string_view> ||
    std::is_same_v<remove_cvref_t<T>, const char*>;

template <typename EnumOrCodec, typename = void>
struct has_enum_mapping : std::false_type {};

template <typename EnumOrCodec>
struct has_enum_mapping<
    EnumOrCodec,
    std::void_t<decltype(detail::enum_mapping_provider<EnumOrCodec>::values())>> : std::true_type {};

template <typename Spec>
constexpr std::string_view logical_name_v = []() constexpr {
    using name_option = typename spec_descriptor<Spec>::name_option;
    if constexpr (std::is_void_v<name_option>) {
        return std::string_view{};
    } else {
        return name_option::value();
    }
}();

template <typename Spec>
[[nodiscard]] constexpr error annotate(error value, const context_policy policy) noexcept {
    return with_context(
        value,
        error_context_entry{spec_descriptor<Spec>::kind, logical_name_v<Spec>, {}},
        policy);
}

template <typename Spec>
[[nodiscard]] constexpr error make_error(
    const error_code code,
    const context_policy policy,
    const std::string_view message = {},
    const int backend_status = 0) noexcept {
    return annotate<Spec>(error{code, {}, message, backend_status}, policy);
}

[[nodiscard]] inline error_code map_deserialization_error(const DeserializationError parse_error) noexcept {
    using Code = DeserializationError::Code;

    switch (parse_error.code()) {
        case Code::EmptyInput:
            return error_code::empty_input;
        case Code::IncompleteInput:
            return error_code::incomplete_input;
        case Code::InvalidInput:
            return error_code::invalid_input;
        case Code::NoMemory:
            return error_code::no_memory;
        case Code::TooDeep:
            return error_code::too_deep;
        case Code::Ok:
            return error_code::ok;
    }

    return error_code::invalid_input;
}

inline DeserializationError deserialize_raw_json(
    JsonDocument& document,
    const std::string_view source,
    const decode_options options) {
    document.clear();

    if (options.nesting_limit == 0U) {
        return deserializeJson(document, source.data(), source.size());
    }

    return deserializeJson(
        document,
        source.data(),
        source.size(),
        DeserializationOption::NestingLimit(options.nesting_limit));
}

template <typename Spec>
[[nodiscard]] constexpr expected_void check_pattern(
    const std::string_view value,
    const context_policy policy,
    const std::string_view failure_message = "pattern mismatch") noexcept {
    using pattern_option = typename spec_descriptor<Spec>::pattern_option;

    if constexpr (std::is_void_v<pattern_option>) {
        (void)value;
        (void)policy;
        (void)failure_message;
        return {};
    } else {
        if (!static_cast<bool>(pattern_option::value(value))) {
            return std::unexpected(make_error<Spec>(error_code::pattern_mismatch, policy, failure_message));
        }

        return {};
    }
}

template <typename T>
struct is_expected_error_code_result : std::false_type {};

template <>
struct is_expected_error_code_result<std::expected<void, error_code>> : std::true_type {};

template <typename T>
inline constexpr bool is_expected_error_code_result_v = is_expected_error_code_result<remove_cvref_t<T>>::value;

template <typename Spec, typename Value>
[[nodiscard]] expected_void run_validator(
    const Value& value,
    const context_policy policy,
    const std::string_view failure_message = "validation failed") {
    using validator_option = typename spec_descriptor<Spec>::validator_option;

    if constexpr (std::is_void_v<validator_option>) {
        (void)value;
        (void)policy;
        (void)failure_message;
        return {};
    } else {
        using result_type = remove_cvref_t<decltype(validator_option::value(value))>;

        if constexpr (std::is_same_v<result_type, bool>) {
            if (validator_option::value(value)) {
                return {};
            }

            return std::unexpected(make_error<Spec>(error_code::validation_failed, policy, failure_message));
        } else if constexpr (is_expected_error_code_result_v<result_type>) {
            auto result = validator_option::value(value);
            if (result.has_value()) {
                return {};
            }

            return std::unexpected(make_error<Spec>(result.error(), policy, failure_message));
        } else if constexpr (std::is_same_v<result_type, expected_void>) {
            auto result = validator_option::value(value);
            if (result.has_value()) {
                return {};
            }

            return std::unexpected(annotate<Spec>(result.error(), policy));
        } else {
            static_assert(
                always_false_v<result_type>,
                "opts::validator_func requires a bool, std::expected<void, json::error_code>, or json::expected_void return type");
        }
    }
}

template <typename Target, typename Spec>
[[nodiscard]] constexpr expected_void check_min_max_value(
    const Target value,
    const context_policy policy,
    const std::string_view failure_message = "value out of range") noexcept {
    using range_option = typename spec_descriptor<Spec>::min_max_value_option;

    if constexpr (std::is_void_v<range_option>) {
        (void)value;
        (void)policy;
        (void)failure_message;
        return {};
    } else {
        using compare_type = long double;
        const auto promoted = static_cast<compare_type>(value);
        const auto min_value = static_cast<compare_type>(range_option::min);
        const auto max_value = static_cast<compare_type>(range_option::max);

        if (promoted < min_value || promoted > max_value) {
            return std::unexpected(make_error<Spec>(error_code::value_out_of_range, policy, failure_message));
        }

        return {};
    }
}

[[nodiscard]] inline bool is_json_number(const JsonVariantConst source) {
    return source.is<float>() || source.is<double>() || source.is<long long>() || source.is<unsigned long long>();
}

template <typename EnumOrCodec>
[[nodiscard]] expected<typename detail::enum_mapping_enum_t<EnumOrCodec>> decode_enum_token(
    const std::string_view token,
    const context_policy policy,
    const node_kind kind,
    const std::string_view logical_name) {
    using mapping_provider = detail::enum_mapping_provider<EnumOrCodec>;

    static_assert(
        has_enum_mapping<EnumOrCodec>::value,
        "enum_string requires either json::traits::enum_strings specialization or a lumalink::json::enum_codec-derived type with values");

    for (const auto& entry : mapping_provider::values()) {
        if (entry.token == token) {
            return entry.value;
        }
    }

    return std::unexpected(with_context(
        error{error_code::enum_string_unknown, {}, "unknown enum token"},
        error_context_entry{kind, logical_name, {}},
        policy));
}

template <typename EnumOrCodec>
[[nodiscard]] expected<std::string_view> encode_enum_value(
    const detail::enum_mapping_enum_t<EnumOrCodec> value,
    const context_policy policy,
    const node_kind kind,
    const std::string_view logical_name) {
    using mapping_provider = detail::enum_mapping_provider<EnumOrCodec>;

    static_assert(
        has_enum_mapping<EnumOrCodec>::value,
        "enum_string requires either json::traits::enum_strings specialization or a lumalink::json::enum_codec-derived type with values");

    for (const auto& entry : mapping_provider::values()) {
        if (entry.value == value) {
            return entry.token;
        }
    }

    return std::unexpected(with_context(
        error{error_code::enum_value_unmapped, {}, "unmapped enum value"},
        error_context_entry{kind, logical_name, {}},
        policy));
}

} // namespace lumalink::json::detail

namespace lumalink::json {

template <typename Spec, typename T, typename Enable = void>
struct decoder {
    static expected<T> decode(JsonVariantConst, const decode_state&) {
        static_assert(detail::always_false_v<Spec, T>, "decoder specialization is not implemented for this Spec/T pair");
    }
};

template <typename Spec, typename T, typename Enable = void>
struct encoder {
    static expected_void encode(const T&, JsonVariant, const encode_state&) {
        static_assert(detail::always_false_v<Spec, T>, "encoder specialization is not implemented for this Spec/T pair");
        return std::unexpected(error{error_code::not_implemented, {}, "encoder specialization missing"});
    }
};

template <typename... Options>
struct decoder<spec::null<Options...>, std::nullptr_t> {
    static expected<std::nullptr_t> decode(const JsonVariantConst source, const decode_state& state) {
        if (!source.isNull()) {
            return std::unexpected(detail::make_error<spec::null<Options...>>(
                error_code::unexpected_type,
                state.context,
                "expected null"));
        }

        const auto validation_result = detail::run_validator<spec::null<Options...>>(nullptr, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        return nullptr;
    }
};

template <typename... Options>
struct encoder<spec::null<Options...>, std::nullptr_t> {
    static expected_void encode(std::nullptr_t value, JsonVariant destination, const encode_state& state) {
        const auto validation_result = detail::run_validator<spec::null<Options...>>(value, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        destination.set(nullptr);
        return {};
    }
};

template <typename... Options>
struct decoder<spec::boolean<Options...>, bool> {
    static expected<bool> decode(const JsonVariantConst source, const decode_state& state) {
        if (!source.is<bool>()) {
            return std::unexpected(detail::make_error<spec::boolean<Options...>>(
                error_code::unexpected_type,
                state.context,
                "expected boolean"));
        }

        const bool value = source.as<bool>();
        const auto validation_result = detail::run_validator<spec::boolean<Options...>>(value, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        return value;
    }
};

template <typename... Options>
struct encoder<spec::boolean<Options...>, bool> {
    static expected_void encode(const bool value, JsonVariant destination, const encode_state& state) {
        const auto validation_result = detail::run_validator<spec::boolean<Options...>>(value, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        destination.set(value);
        return {};
    }
};

template <typename... Options, typename T>
struct decoder<spec::integer<Options...>, T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
    static expected<T> decode(const JsonVariantConst source, const decode_state& state) {
        T value{};

        if constexpr (std::is_signed_v<T>) {
            if (source.is<long long>()) {
                const auto raw = source.as<long long>();
                if (raw < static_cast<long long>(std::numeric_limits<T>::min()) ||
                    raw > static_cast<long long>(std::numeric_limits<T>::max())) {
                    return std::unexpected(detail::make_error<spec::integer<Options...>>(
                        error_code::value_out_of_range,
                        state.context,
                        "signed integer out of range"));
                }

                value = static_cast<T>(raw);
            } else if (source.is<unsigned long long>()) {
                const auto raw = source.as<unsigned long long>();
                if (raw > static_cast<unsigned long long>(std::numeric_limits<T>::max())) {
                    return std::unexpected(detail::make_error<spec::integer<Options...>>(
                        error_code::value_out_of_range,
                        state.context,
                        "signed integer out of range"));
                }

                value = static_cast<T>(raw);
            } else {
                return std::unexpected(detail::make_error<spec::integer<Options...>>(
                    error_code::unexpected_type,
                    state.context,
                    "expected integer"));
            }
        } else {
            if (source.is<unsigned long long>()) {
                const auto raw = source.as<unsigned long long>();
                if (raw > static_cast<unsigned long long>(std::numeric_limits<T>::max())) {
                    return std::unexpected(detail::make_error<spec::integer<Options...>>(
                        error_code::value_out_of_range,
                        state.context,
                        "unsigned integer out of range"));
                }

                value = static_cast<T>(raw);
            } else if (source.is<long long>()) {
                const auto raw = source.as<long long>();
                if (raw < 0 || static_cast<unsigned long long>(raw) > std::numeric_limits<T>::max()) {
                    return std::unexpected(detail::make_error<spec::integer<Options...>>(
                        error_code::value_out_of_range,
                        state.context,
                        "unsigned integer out of range"));
                }

                value = static_cast<T>(raw);
            } else {
                return std::unexpected(detail::make_error<spec::integer<Options...>>(
                    error_code::unexpected_type,
                    state.context,
                    "expected integer"));
            }
        }

        const auto bounds_result = detail::check_min_max_value<T, spec::integer<Options...>>(value, state.context);
        if (!bounds_result.has_value()) {
            return std::unexpected(bounds_result.error());
        }

        const auto validation_result = detail::run_validator<spec::integer<Options...>>(value, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        return value;
    }
};

template <typename... Options, typename T>
struct encoder<spec::integer<Options...>, T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>>> {
    static expected_void encode(const T value, JsonVariant destination, const encode_state& state) {
        const auto bounds_result = detail::check_min_max_value<T, spec::integer<Options...>>(value, state.context);
        if (!bounds_result.has_value()) {
            return std::unexpected(bounds_result.error());
        }

        const auto validation_result = detail::run_validator<spec::integer<Options...>>(value, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        destination.set(value);
        return {};
    }
};

template <typename... Options, typename T>
struct decoder<spec::number<Options...>, T, std::enable_if_t<std::is_floating_point_v<T>>> {
    static expected<T> decode(const JsonVariantConst source, const decode_state& state) {
        if (!detail::is_json_number(source)) {
            return std::unexpected(detail::make_error<spec::number<Options...>>(
                error_code::unexpected_type,
                state.context,
                "expected number"));
        }

        const T value = source.as<T>();
        const auto bounds_result = detail::check_min_max_value<T, spec::number<Options...>>(value, state.context);
        if (!bounds_result.has_value()) {
            return std::unexpected(bounds_result.error());
        }

        const auto validation_result = detail::run_validator<spec::number<Options...>>(value, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        return value;
    }
};

template <typename... Options, typename T>
struct encoder<spec::number<Options...>, T, std::enable_if_t<std::is_floating_point_v<T>>> {
    static expected_void encode(const T value, JsonVariant destination, const encode_state& state) {
        const auto bounds_result = detail::check_min_max_value<T, spec::number<Options...>>(value, state.context);
        if (!bounds_result.has_value()) {
            return std::unexpected(bounds_result.error());
        }

        const auto validation_result = detail::run_validator<spec::number<Options...>>(value, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        destination.set(value);
        return {};
    }
};

template <typename... Options, typename T>
struct decoder<spec::string<Options...>, T, std::enable_if_t<detail::is_supported_string_target_v<T>>> {
    static expected<T> decode(const JsonVariantConst source, const decode_state& state) {
        if (!source.is<const char*>()) {
            return std::unexpected(detail::make_error<spec::string<Options...>>(
                error_code::unexpected_type,
                state.context,
                "expected string"));
        }

        const char* raw = source.as<const char*>();
        const std::string_view value = raw == nullptr ? std::string_view{} : std::string_view{raw, std::strlen(raw)};

        const auto pattern_result = detail::check_pattern<spec::string<Options...>>(value, state.context);
        if (!pattern_result.has_value()) {
            return std::unexpected(pattern_result.error());
        }

        if constexpr (std::is_same_v<detail::remove_cvref_t<T>, std::string>) {
            T decoded{value};
            const auto validation_result = detail::run_validator<spec::string<Options...>>(decoded, state.context);
            if (!validation_result.has_value()) {
                return std::unexpected(validation_result.error());
            }

            return decoded;
        } else if constexpr (std::is_same_v<detail::remove_cvref_t<T>, std::string_view>) {
            T decoded{value};
            const auto validation_result = detail::run_validator<spec::string<Options...>>(decoded, state.context);
            if (!validation_result.has_value()) {
                return std::unexpected(validation_result.error());
            }

            return decoded;
        } else {
            T decoded = raw;
            const auto validation_result = detail::run_validator<spec::string<Options...>>(decoded, state.context);
            if (!validation_result.has_value()) {
                return std::unexpected(validation_result.error());
            }

            return decoded;
        }
    }
};

template <typename... Options, typename T>
struct encoder<spec::string<Options...>, T, std::enable_if_t<detail::is_supported_string_target_v<T>>> {
    static expected_void encode(const T& value, JsonVariant destination, const encode_state& state) {
        std::string_view view{};

        if constexpr (std::is_same_v<detail::remove_cvref_t<T>, std::string>) {
            view = value;
        } else if constexpr (std::is_same_v<detail::remove_cvref_t<T>, std::string_view>) {
            view = value;
        } else {
            if (value == nullptr) {
                return std::unexpected(detail::make_error<spec::string<Options...>>(
                    error_code::unexpected_type,
                    state.context,
                    "null string pointer"));
            }

            view = std::string_view{value, std::strlen(value)};
        }

        const auto pattern_result = detail::check_pattern<spec::string<Options...>>(view, state.context);
        if (!pattern_result.has_value()) {
            return std::unexpected(pattern_result.error());
        }

        const auto validation_result = detail::run_validator<spec::string<Options...>>(value, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        destination.set(std::string(view));
        return {};
    }
};

template <typename EnumOrCodec, typename... Options>
struct decoder<spec::enum_string<EnumOrCodec, Options...>, detail::enum_mapping_enum_t<EnumOrCodec>> {
    using enum_type = detail::enum_mapping_enum_t<EnumOrCodec>;

    static expected<enum_type> decode(const JsonVariantConst source, const decode_state& state) {
        if (!source.is<const char*>()) {
            return std::unexpected(detail::make_error<spec::enum_string<EnumOrCodec, Options...>>(
                error_code::unexpected_type,
                state.context,
                "expected enum string token"));
        }

        const char* raw = source.as<const char*>();
        if (raw == nullptr) {
            return std::unexpected(detail::make_error<spec::enum_string<EnumOrCodec, Options...>>(
                error_code::unexpected_type,
                state.context,
                "expected enum string token"));
        }

        auto decoded = detail::decode_enum_token<EnumOrCodec>(
            std::string_view{raw, std::strlen(raw)},
            state.context,
            node_kind::enum_string,
            detail::logical_name_v<spec::enum_string<EnumOrCodec, Options...>>);
        if (!decoded.has_value()) {
            return std::unexpected(decoded.error());
        }

        const auto validation_result = detail::run_validator<spec::enum_string<EnumOrCodec, Options...>>(
            *decoded,
            state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        return decoded;
    }
};

template <typename EnumOrCodec, typename... Options>
struct encoder<spec::enum_string<EnumOrCodec, Options...>, detail::enum_mapping_enum_t<EnumOrCodec>> {
    using enum_type = detail::enum_mapping_enum_t<EnumOrCodec>;

    static expected_void encode(const enum_type value, JsonVariant destination, const encode_state& state) {
        const auto token_result = detail::encode_enum_value<EnumOrCodec>(
            value,
            state.context,
            node_kind::enum_string,
            detail::logical_name_v<spec::enum_string<EnumOrCodec, Options...>>);
        if (!token_result.has_value()) {
            return std::unexpected(token_result.error());
        }

        const auto validation_result = detail::run_validator<spec::enum_string<EnumOrCodec, Options...>>(
            value,
            state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        destination.set(std::string(*token_result));
        return {};
    }
};

template <typename... Options>
struct decoder<spec::any<Options...>, JsonVariantConst> {
    static expected<JsonVariantConst> decode(const JsonVariantConst source, const decode_state& state) {
        const auto validation_result = detail::run_validator<spec::any<Options...>>(source, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        return expected<JsonVariantConst>{source};
    }
};

template <typename... Options>
struct encoder<spec::any<Options...>, JsonVariantConst> {
    static expected_void encode(const JsonVariantConst& value, JsonVariant destination, const encode_state& state) {
        const auto validation_result = detail::run_validator<spec::any<Options...>>(value, state.context);
        if (!validation_result.has_value()) {
            return std::unexpected(validation_result.error());
        }

        destination.set(value);
        return {};
    }
};

template <typename InnerSpec, typename Codec, typename T>
struct decoder<spec::with_codec<InnerSpec, Codec>, T> {
    static expected<T> decode(const JsonVariantConst source, const decode_state& state) {
        static_assert(
            detail::has_wrapped_codec_decode_v<Codec, InnerSpec, T>,
            "spec::with_codec requires Codec::template decode<InnerSpec, T>(JsonVariantConst, const decode_state&)"
        );

        using result_type = detail::remove_cvref_t<decltype(Codec::template decode<InnerSpec, T>(source, state))>;
        static_assert(
            std::is_same_v<result_type, expected<T>>,
            "spec::with_codec decode must return json::expected<T>"
        );

        return Codec::template decode<InnerSpec, T>(source, state);
    }
};

template <typename InnerSpec, typename Codec, typename T>
struct encoder<spec::with_codec<InnerSpec, Codec>, T> {
    static expected_void encode(const T& value, JsonVariant destination, const encode_state& state) {
        static_assert(
            detail::has_wrapped_codec_encode_v<Codec, InnerSpec, T>,
            "spec::with_codec requires Codec::template encode<InnerSpec, T>(const T&, JsonVariant, const encode_state&)"
        );

        using result_type =
            detail::remove_cvref_t<decltype(Codec::template encode<InnerSpec, T>(value, destination, state))>;
        static_assert(
            std::is_same_v<result_type, expected_void>,
            "spec::with_codec encode must return json::expected_void"
        );

        return Codec::template encode<InnerSpec, T>(value, destination, state);
    }
};

// Support decoding/encoding a whole JsonDocument as a field/property value.
// This allows struct members like `JsonDocument foo;` to round-trip through
// the codec system. We copy the JSON subtree into a temporary document when
// decoding and copy the caller-provided document's root when encoding.
template <typename Spec>
struct decoder<Spec, JsonDocument> {
    static expected<JsonDocument> decode(const JsonVariantConst source, const decode_state& /*state*/) {
        JsonDocument doc;
        JsonVariant dst = doc.to<JsonVariant>();
        dst.set(source);
        return expected<JsonDocument>{std::move(doc)};
    }
};

template <typename Spec>
struct encoder<Spec, JsonDocument> {
    static expected_void encode(const JsonDocument& value, JsonVariant destination, const encode_state& /*state*/) {
        // Copy the provided document's root into destination using a const view
        destination.set(value.as<JsonVariantConst>());
        return {};
    }
};

template <typename Target, typename Spec>
expected<Target> deserialize(const JsonVariantConst source, const decode_options options = {}) {
    return decoder<Spec, Target>::decode(source, decode_state{options.context});
}

template <typename Target, typename Spec>
expected<Target> deserialize(const JsonDocument& document, const decode_options options = {}) {
    return deserialize<Target, Spec>(document.as<JsonVariantConst>(), options);
}

template <typename Target, typename Spec>
expected<Target> deserialize(const std::string_view source, JsonDocument& document, const decode_options options = {}) {
    const auto parse_error = detail::deserialize_raw_json(document, source, options);
    if (parse_error) {
        return std::unexpected(error{
            detail::map_deserialization_error(parse_error),
            {},
            parse_error.c_str(),
            static_cast<int>(parse_error.code())});
    }

    return deserialize<Target, Spec>(document.as<JsonVariantConst>(), options);
}

template <typename Target, typename Spec>
expected<Target> deserialize(const std::string_view source, const decode_options options = {}) {
    JsonDocument document;
    return deserialize<Target, Spec>(source, document, options);
}

template <typename Spec, typename Source>
expected_void serialize(const Source& value, JsonVariant destination, const encode_options options = {}) {
    return encoder<Spec, detail::remove_cvref_t<Source>>::encode(value, destination, encode_state{options.context});
}

template <typename Spec, typename Source>
expected_void serialize(const Source& value, JsonDocument& document, const encode_options options = {}) {
    document.clear();
    JsonVariant destination = document.to<JsonVariant>();
    return serialize<Spec>(value, destination, options);
}

} // namespace lumalink::json
