#include <ArduinoJson.h>
#include <unity.h>

#include <any>
#include <array>
#include <charconv>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <vector>

#include <LumaLinkJson.h>

#include "support/test_support.h"

namespace {

struct project_token {
    int value{};
};

enum class project_wire_mode : unsigned char {
    standby,
    active,
    maintenance,
};

struct project_endpoint {
    project_token token;
    project_wire_mode mode;
};

struct stop_color {
    uint8_t R{};
    uint8_t G{};
    uint8_t B{};
};

struct stop {
    uint16_t index{};
    stop_color color{};
};

struct dynamic_payload_holder {
    std::any payload;
};

enum class tagged_payload_kind : unsigned char {
    counter,
    message,
    endpoint,
};

struct tagged_payload_base {
    explicit tagged_payload_base(const tagged_payload_kind discriminator_value) : discriminator(discriminator_value) {}
    virtual ~tagged_payload_base() = default;

    tagged_payload_kind discriminator;
};

struct project_wire_mode_codec : lumalink::json::enum_codec<project_wire_mode_codec, project_wire_mode> {
    static constexpr std::array<lumalink::json::traits::enum_mapping_entry<project_wire_mode>, 3> values{{
        {"standby", project_wire_mode::standby},
        {"active", project_wire_mode::active},
        {"maintenance", project_wire_mode::maintenance},
    }};
};

struct tagged_counter_payload : tagged_payload_base {
    tagged_counter_payload() : tagged_payload_base(tagged_payload_kind::counter) {}
    explicit tagged_counter_payload(const int count_value)
        : tagged_payload_base(tagged_payload_kind::counter), count(count_value) {}

    int count{};
};

struct tagged_message_payload : tagged_payload_base {
    tagged_message_payload() : tagged_payload_base(tagged_payload_kind::message) {}
    tagged_message_payload(std::string text_value, const bool urgent_value)
        : tagged_payload_base(tagged_payload_kind::message), text(std::move(text_value)), urgent(urgent_value) {}

    std::string text;
    bool urgent{};
};

struct tagged_endpoint_payload : tagged_payload_base {
    tagged_endpoint_payload() : tagged_payload_base(tagged_payload_kind::endpoint) {}
    tagged_endpoint_payload(const project_token token_value, const project_wire_mode mode_value)
        : tagged_payload_base(tagged_payload_kind::endpoint), token(token_value), mode(mode_value) {}

    project_token token;
    project_wire_mode mode{project_wire_mode::standby};
};

using project_token_spec = lumalink::json::spec::string<>;
using project_mode_spec = lumalink::json::spec::enum_string<project_wire_mode_codec>;
using project_endpoint_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"token", project_token_spec>,
    lumalink::json::spec::field<"mode", project_mode_spec>>;
using stop_list_spec = lumalink::json::spec::string<>;
using tagged_counter_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"count", lumalink::json::spec::integer<>>>;
using tagged_message_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"text", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"urgent", lumalink::json::spec::boolean<>>>;
using tagged_endpoint_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"token", project_token_spec>,
    lumalink::json::spec::field<"mode", project_mode_spec>>;

lumalink::json::expected<project_token> parse_project_token(
    std::string_view token,
    lumalink::json::context_policy policy);

constexpr std::string_view tagged_payload_kind_token(const tagged_payload_kind value) {
    switch (value) {
        case tagged_payload_kind::counter:
            return "counter";
        case tagged_payload_kind::message:
            return "message";
        case tagged_payload_kind::endpoint:
            return "endpoint";
    }

    return {};
}

struct tagged_any_codec {
    template <typename InnerSpec, typename Payload, typename PayloadSpec>
    static lumalink::json::expected<std::any> decode_payload(
        const JsonVariantConst value_source,
        const lumalink::json::decode_state& state) {
        auto decoded = lumalink::json::deserialize<Payload, PayloadSpec>(
            value_source,
            lumalink::json::decode_options{state.context});
        if (!decoded.has_value()) {
            return std::unexpected(lumalink::json::detail::annotate<InnerSpec>(decoded.error(), state.context));
        }

        return std::any{std::shared_ptr<tagged_payload_base>(std::make_shared<Payload>(std::move(*decoded)))};
    }

    template <typename InnerSpec, typename Payload, typename PayloadSpec>
    static lumalink::json::expected_void encode_payload(
        const Payload& payload,
        JsonObject object,
        const lumalink::json::encode_state& state) {
        object["kind"] = std::string(tagged_payload_kind_token(payload.discriminator));

        JsonVariant value_destination = object["value"].template to<JsonVariant>();
        auto encoded = lumalink::json::serialize<PayloadSpec>(payload, value_destination, lumalink::json::encode_options{state.context});
        if (!encoded.has_value()) {
            return std::unexpected(lumalink::json::detail::annotate<InnerSpec>(encoded.error(), state.context));
        }

        return {};
    }

    template <typename InnerSpec, typename T>
    static lumalink::json::expected<T> decode(
        const JsonVariantConst source,
        const lumalink::json::decode_state& state) {
        static_assert(std::is_same_v<T, std::any>, "tagged_any_codec only supports std::any");

        if (!source.is<JsonObjectConst>()) {
            return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
                lumalink::json::error_code::unexpected_type,
                state.context,
                "expected tagged any object"));
        }

        const JsonObjectConst object = source.as<JsonObjectConst>();
        const JsonVariantConst kind_source = object["kind"];
        const JsonVariantConst value_source = object["value"];

        if (kind_source.isUnbound() || !kind_source.is<const char*>()) {
            return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
                lumalink::json::error_code::unexpected_type,
                state.context,
                "expected tagged any kind"));
        }

        if (value_source.isUnbound()) {
            return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
                lumalink::json::error_code::missing_field,
                state.context,
                "expected tagged any value"));
        }

        const std::string_view kind = kind_source.as<const char*>();
        if (kind == tagged_payload_kind_token(tagged_payload_kind::counter)) {
            auto decoded = decode_payload<InnerSpec, tagged_counter_payload, tagged_counter_spec>(value_source, state);
            if (!decoded.has_value()) {
                return std::unexpected(decoded.error());
            }

            return T{std::move(*decoded)};
        }

        if (kind == tagged_payload_kind_token(tagged_payload_kind::message)) {
            auto decoded = decode_payload<InnerSpec, tagged_message_payload, tagged_message_spec>(value_source, state);
            if (!decoded.has_value()) {
                return std::unexpected(decoded.error());
            }

            return T{std::move(*decoded)};
        }

        if (kind == tagged_payload_kind_token(tagged_payload_kind::endpoint)) {
            auto decoded = decode_payload<InnerSpec, tagged_endpoint_payload, tagged_endpoint_spec>(value_source, state);
            if (!decoded.has_value()) {
                return std::unexpected(decoded.error());
            }

            return T{std::move(*decoded)};
        }

        return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
            lumalink::json::error_code::validation_failed,
            state.context,
            "unsupported tagged any kind"));
    }

    template <typename InnerSpec, typename T>
    static lumalink::json::expected_void encode(
        const T& value,
        JsonVariant destination,
        const lumalink::json::encode_state& state) {
        static_assert(std::is_same_v<T, std::any>, "tagged_any_codec only supports std::any");

        JsonObject object = destination.to<JsonObject>();

        if (!value.has_value()) {
            return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
                lumalink::json::error_code::validation_failed,
                state.context,
                "empty std::any payload"));
        }

        const auto* payload = std::any_cast<std::shared_ptr<tagged_payload_base>>(&value);
        if (payload == nullptr || !static_cast<bool>(*payload)) {
            return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
                lumalink::json::error_code::validation_failed,
                state.context,
                "expected shared tagged payload pointer"));
        }

        switch ((*payload)->discriminator) {
            case tagged_payload_kind::counter: {
                const auto derived = std::dynamic_pointer_cast<tagged_counter_payload>(*payload);
                if (!static_cast<bool>(derived)) {
                    return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
                        lumalink::json::error_code::validation_failed,
                        state.context,
                        "counter discriminator did not match payload type"));
                }

                return encode_payload<InnerSpec, tagged_counter_payload, tagged_counter_spec>(*derived, object, state);
            }
            case tagged_payload_kind::message: {
                const auto derived = std::dynamic_pointer_cast<tagged_message_payload>(*payload);
                if (!static_cast<bool>(derived)) {
                    return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
                        lumalink::json::error_code::validation_failed,
                        state.context,
                        "message discriminator did not match payload type"));
                }

                return encode_payload<InnerSpec, tagged_message_payload, tagged_message_spec>(*derived, object, state);
            }
            case tagged_payload_kind::endpoint: {
                const auto derived = std::dynamic_pointer_cast<tagged_endpoint_payload>(*payload);
                if (!static_cast<bool>(derived)) {
                    return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
                        lumalink::json::error_code::validation_failed,
                        state.context,
                        "endpoint discriminator did not match payload type"));
                }

                return encode_payload<InnerSpec, tagged_endpoint_payload, tagged_endpoint_spec>(*derived, object, state);
            }
        }

        return std::unexpected(lumalink::json::detail::make_error<InnerSpec>(
            lumalink::json::error_code::validation_failed,
            state.context,
            "unsupported std::any payload type"));
    }
};

using tagged_any_spec = lumalink::json::spec::with_codec<lumalink::json::spec::any<>, tagged_any_codec>;
using dynamic_payload_holder_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"payload", tagged_any_spec>>;

lumalink::json::expected<project_token> parse_project_token(
    const std::string_view token,
    const lumalink::json::context_policy policy) {
    constexpr std::string_view prefix{"TK-"};
    if (!token.starts_with(prefix) || token.size() == prefix.size()) {
        return std::unexpected(lumalink::json::detail::make_error<project_token_spec>(
            lumalink::json::error_code::validation_failed,
            policy,
            "invalid project token"));
    }

    const std::string_view digits = token.substr(prefix.size());
    int parsed = 0;
    const auto [ptr, error_code] = std::from_chars(digits.data(), digits.data() + digits.size(), parsed);
    if (error_code != std::errc{} || ptr != digits.data() + digits.size()) {
        return std::unexpected(lumalink::json::detail::make_error<project_token_spec>(
            lumalink::json::error_code::validation_failed,
            policy,
            "invalid project token"));
    }

    return project_token{parsed};
}

lumalink::json::expected<uint8_t> parse_hex_byte(
    const std::string_view token,
    const lumalink::json::context_policy policy) {
    if (token.size() != 2U) {
        return std::unexpected(lumalink::json::detail::make_error<stop_list_spec>(
            lumalink::json::error_code::validation_failed,
            policy,
            "invalid stop color"));
    }

    unsigned int parsed = 0U;
    const auto [ptr, error_code] = std::from_chars(token.data(), token.data() + token.size(), parsed, 16);
    if (error_code != std::errc{} || ptr != token.data() + token.size() || parsed > 0xFFU) {
        return std::unexpected(lumalink::json::detail::make_error<stop_list_spec>(
            lumalink::json::error_code::validation_failed,
            policy,
            "invalid stop color"));
    }

    return static_cast<uint8_t>(parsed);
}

lumalink::json::expected<stop_color> parse_stop_color(
    const std::string_view token,
    const lumalink::json::context_policy policy) {
    std::string_view hex = token;
    if (!hex.empty() && hex.front() == '#') {
        hex.remove_prefix(1U);
    }

    if (hex.size() != 6U) {
        return std::unexpected(lumalink::json::detail::make_error<stop_list_spec>(
            lumalink::json::error_code::validation_failed,
            policy,
            "invalid stop color"));
    }

    auto red = parse_hex_byte(hex.substr(0U, 2U), policy);
    if (!red.has_value()) {
        return std::unexpected(red.error());
    }

    auto green = parse_hex_byte(hex.substr(2U, 2U), policy);
    if (!green.has_value()) {
        return std::unexpected(green.error());
    }

    auto blue = parse_hex_byte(hex.substr(4U, 2U), policy);
    if (!blue.has_value()) {
        return std::unexpected(blue.error());
    }

    return stop_color{*red, *green, *blue};
}

lumalink::json::expected<stop> parse_stop_entry(
    const std::string_view token,
    const lumalink::json::context_policy policy) {
    const size_t comma_index = token.find(',');
    if (comma_index == std::string_view::npos || comma_index == 0U || comma_index + 1U >= token.size()) {
        return std::unexpected(lumalink::json::detail::make_error<stop_list_spec>(
            lumalink::json::error_code::validation_failed,
            policy,
            "invalid stop entry"));
    }

    const std::string_view index_token = token.substr(0U, comma_index);
    const std::string_view color_token = token.substr(comma_index + 1U);

    unsigned int parsed_index = 0U;
    const auto [ptr, error_code] = std::from_chars(
        index_token.data(),
        index_token.data() + index_token.size(),
        parsed_index);
    if (error_code != std::errc{} || ptr != index_token.data() + index_token.size() ||
        parsed_index > std::numeric_limits<uint16_t>::max()) {
        return std::unexpected(lumalink::json::detail::make_error<stop_list_spec>(
            lumalink::json::error_code::validation_failed,
            policy,
            "invalid stop index"));
    }

    auto parsed_color = parse_stop_color(color_token, policy);
    if (!parsed_color.has_value()) {
        return std::unexpected(parsed_color.error());
    }

    return stop{static_cast<uint16_t>(parsed_index), *parsed_color};
}

lumalink::json::expected<std::vector<stop>> parse_stop_list(
    const std::string_view token,
    const lumalink::json::context_policy policy) {
    std::vector<stop> parsed_stops;
    if (token.empty()) {
        return parsed_stops;
    }

    std::string_view remaining = token;
    while (!remaining.empty()) {
        const size_t separator_index = remaining.find('|');
        const std::string_view entry = separator_index == std::string_view::npos
            ? remaining
            : remaining.substr(0U, separator_index);

        if (entry.empty()) {
            return std::unexpected(lumalink::json::detail::make_error<stop_list_spec>(
                lumalink::json::error_code::validation_failed,
                policy,
                "invalid stop entry"));
        }

        auto parsed_entry = parse_stop_entry(entry, policy);
        if (!parsed_entry.has_value()) {
            return std::unexpected(parsed_entry.error());
        }

        parsed_stops.push_back(*parsed_entry);

        if (separator_index == std::string_view::npos) {
            break;
        }

        remaining.remove_prefix(separator_index + 1U);
    }

    return parsed_stops;
}

std::string serialize_document_or_fail(const JsonDocument& document) {
    std::string serialized;
    const size_t written = serializeJson(document, serialized);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, static_cast<uint32_t>(written));
    return serialized;
}

} // namespace

namespace lumalink::json {

template <>
struct decoder<spec::string<>, project_token> {
    static expected<project_token> decode(const JsonVariantConst source, const decode_state& state) {
        if (!source.is<const char*>()) {
            return std::unexpected(detail::make_error<spec::string<>>(
                error_code::unexpected_type,
                state.context,
                "expected token string"));
        }

        const char* raw = source.as<const char*>();
        if (raw == nullptr) {
            return std::unexpected(detail::make_error<spec::string<>>(
                error_code::unexpected_type,
                state.context,
                "expected token string"));
        }

        return parse_project_token(std::string_view{raw, std::strlen(raw)}, state.context);
    }
};

template <>
struct encoder<spec::string<>, project_token> {
    static expected_void encode(const project_token& value, JsonVariant destination, const encode_state& state) {
        if (value.value < 0) {
            return std::unexpected(detail::make_error<spec::string<>>(
                error_code::validation_failed,
                state.context,
                "project token must be non-negative"));
        }

        destination.set(std::string("TK-") + std::to_string(value.value));
        return {};
    }
};

template <>
struct decoder<spec::string<>, std::vector<stop>> {
    static expected<std::vector<stop>> decode(const JsonVariantConst source, const decode_state& state) {
        if (!source.is<const char*>()) {
            return std::unexpected(detail::make_error<spec::string<>>(
                error_code::unexpected_type,
                state.context,
                "expected stop-list string"));
        }

        const char* raw = source.as<const char*>();
        if (raw == nullptr) {
            return std::unexpected(detail::make_error<spec::string<>>(
                error_code::unexpected_type,
                state.context,
                "expected stop-list string"));
        }

        return parse_stop_list(std::string_view{raw, std::strlen(raw)}, state.context);
    }
};

} // namespace lumalink::json

namespace lumalink::json::traits {

template <>
struct object_fields<tagged_counter_payload> {
    static constexpr auto members = std::make_tuple(&tagged_counter_payload::count);
};

template <>
struct object_fields<tagged_message_payload> {
    static constexpr auto members = std::make_tuple(&tagged_message_payload::text, &tagged_message_payload::urgent);
};

template <>
struct object_fields<tagged_endpoint_payload> {
    static constexpr auto members = std::make_tuple(&tagged_endpoint_payload::token, &tagged_endpoint_payload::mode);
};

} // namespace lumalink::json::traits

void test_ext_01_custom_decoder_specialization_supports_non_standard_targets() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"("TK-42")");

    const auto result = lumalink::json::deserialize<project_token, project_token_spec>(fixture.root());
    TEST_ASSERT_EQUAL(42, lumalink::json::test_support::assert_expected_success(result).value);
}

void test_ext_01b_custom_decoder_parses_indexed_color_stop_lists() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"("0,#FF0000|128,#00FF40|1024,#112233")");

    const auto result = lumalink::json::deserialize<std::vector<stop>, stop_list_spec>(fixture.root());
    const auto& value = lumalink::json::test_support::assert_expected_success(result);
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(value.size()));

    TEST_ASSERT_EQUAL_UINT16(0U, value[0].index);
    TEST_ASSERT_EQUAL_UINT8(255U, value[0].color.R);
    TEST_ASSERT_EQUAL_UINT8(0U, value[0].color.G);
    TEST_ASSERT_EQUAL_UINT8(0U, value[0].color.B);

    TEST_ASSERT_EQUAL_UINT16(128U, value[1].index);
    TEST_ASSERT_EQUAL_UINT8(0U, value[1].color.R);
    TEST_ASSERT_EQUAL_UINT8(255U, value[1].color.G);
    TEST_ASSERT_EQUAL_UINT8(64U, value[1].color.B);

    TEST_ASSERT_EQUAL_UINT16(1024U, value[2].index);
    TEST_ASSERT_EQUAL_UINT8(17U, value[2].color.R);
    TEST_ASSERT_EQUAL_UINT8(34U, value[2].color.G);
    TEST_ASSERT_EQUAL_UINT8(51U, value[2].color.B);
}

void test_ext_02_custom_encoder_specialization_supports_non_standard_sources() {
    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<project_token_spec>(project_token{17}, encoded));

    TEST_ASSERT_EQUAL_STRING("\"TK-17\"", serialize_document_or_fail(encoded).c_str());
}

void test_ext_03_custom_enum_mapping_traits_support_project_specific_enums() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"("maintenance")");

    const auto decoded = lumalink::json::deserialize<project_wire_mode, project_mode_spec>(fixture.root());
    TEST_ASSERT_EQUAL(
        static_cast<int>(project_wire_mode::maintenance),
        static_cast<int>(lumalink::json::test_support::assert_expected_success(decoded)));

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<project_mode_spec>(project_wire_mode::active, encoded));
    TEST_ASSERT_EQUAL_STRING("\"active\"", serialize_document_or_fail(encoded).c_str());
}

void test_ext_03a_enum_codec_lookup_helpers_resolve_mapped_entries() {
    const auto decoded = project_wire_mode_codec::value_from_token("active");
    TEST_ASSERT_TRUE(decoded.has_value());
    TEST_ASSERT_EQUAL(static_cast<int>(project_wire_mode::active), static_cast<int>(*decoded));

    const auto encoded = project_wire_mode_codec::token_from_value(project_wire_mode::maintenance);
    TEST_ASSERT_TRUE(encoded.has_value());
    TEST_ASSERT_EQUAL_STRING("maintenance", std::string(*encoded).c_str());
}

void test_ext_03b_enum_codec_lookup_helpers_report_missing_entries() {
    TEST_ASSERT_FALSE(project_wire_mode_codec::value_from_token("offline").has_value());
    TEST_ASSERT_FALSE(project_wire_mode_codec::token_from_value(static_cast<project_wire_mode>(0xFF)).has_value());
}

void test_ext_04_custom_converter_failures_propagate_expected_errors() {
    lumalink::json::test_support::native_fixture decode_fixture;
    decode_fixture.parse_or_fail(R"({"token":"bad","mode":"standby"})");

    const auto decoded = lumalink::json::deserialize<project_endpoint, project_endpoint_spec>(decode_fixture.root());
    const auto& decode_failure =
        lumalink::json::test_support::assert_expected_error(decoded, lumalink::json::error_code::validation_failed);
    lumalink::json::test_support::assert_error_context_depth(decode_failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(decode_failure, 0U, lumalink::json::node_kind::string);
    lumalink::json::test_support::assert_error_context_entry(
        decode_failure,
        1U,
        lumalink::json::node_kind::field,
        "token");
    lumalink::json::test_support::assert_error_context_entry(decode_failure, 2U, lumalink::json::node_kind::object);

    JsonDocument encoded;
    const auto encoded_result = lumalink::json::serialize<project_endpoint_spec>(
        project_endpoint{project_token{-1}, project_wire_mode::standby},
        encoded);
    const auto& encode_failure = lumalink::json::test_support::assert_expected_error(
        encoded_result,
        lumalink::json::error_code::validation_failed);
    lumalink::json::test_support::assert_error_context_depth(encode_failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(encode_failure, 0U, lumalink::json::node_kind::string);
    lumalink::json::test_support::assert_error_context_entry(
        encode_failure,
        1U,
        lumalink::json::node_kind::field,
        "token");
    lumalink::json::test_support::assert_error_context_entry(encode_failure, 2U, lumalink::json::node_kind::object);
}

void test_ext_05_codec_wrapped_spec_supports_std_any_decode() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"payload":{"kind":"endpoint","value":{"token":"TK-33","mode":"active"}}})");

    const auto result = lumalink::json::deserialize<dynamic_payload_holder, dynamic_payload_holder_spec>(fixture.root());
    const auto& holder = lumalink::json::test_support::assert_expected_success(result);

    TEST_ASSERT_TRUE(holder.payload.has_value());
    TEST_ASSERT_TRUE(holder.payload.type() == typeid(std::shared_ptr<tagged_payload_base>));

    const auto payload = std::any_cast<std::shared_ptr<tagged_payload_base>>(holder.payload);
    TEST_ASSERT_TRUE(static_cast<bool>(payload));
    TEST_ASSERT_EQUAL(
        static_cast<int>(tagged_payload_kind::endpoint),
        static_cast<int>(payload->discriminator));

    const auto endpoint = std::dynamic_pointer_cast<tagged_endpoint_payload>(payload);
    TEST_ASSERT_TRUE(static_cast<bool>(endpoint));
    TEST_ASSERT_EQUAL(33, endpoint->token.value);
    TEST_ASSERT_EQUAL(static_cast<int>(project_wire_mode::active), static_cast<int>(endpoint->mode));
}

void test_ext_06_codec_wrapped_spec_supports_std_any_encode() {
    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(lumalink::json::serialize<dynamic_payload_holder_spec>(
        dynamic_payload_holder{std::any{std::shared_ptr<tagged_payload_base>(
            std::make_shared<tagged_message_payload>("READY", true))}},
        encoded));

    TEST_ASSERT_EQUAL_STRING(
        "{\"payload\":{\"kind\":\"message\",\"value\":{\"text\":\"READY\",\"urgent\":true}}}",
        serialize_document_or_fail(encoded).c_str());
}

void test_ext_07_codec_wrapped_spec_supports_second_tagged_structure() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"payload":{"kind":"counter","value":{"count":12}}})");

    const auto result = lumalink::json::deserialize<dynamic_payload_holder, dynamic_payload_holder_spec>(fixture.root());
    const auto& holder = lumalink::json::test_support::assert_expected_success(result);

    const auto payload = std::any_cast<std::shared_ptr<tagged_payload_base>>(holder.payload);
    TEST_ASSERT_TRUE(static_cast<bool>(payload));
    TEST_ASSERT_EQUAL(
        static_cast<int>(tagged_payload_kind::counter),
        static_cast<int>(payload->discriminator));

    const auto counter = std::dynamic_pointer_cast<tagged_counter_payload>(payload);
    TEST_ASSERT_TRUE(static_cast<bool>(counter));
    TEST_ASSERT_EQUAL(12, counter->count);
}

void test_ext_08_codec_wrapped_spec_propagates_std_any_failures() {
    lumalink::json::test_support::native_fixture decode_fixture;
    decode_fixture.parse_or_fail(R"({"payload":{"kind":"mystery","value":17}})");

    const auto decoded = lumalink::json::deserialize<dynamic_payload_holder, dynamic_payload_holder_spec>(decode_fixture.root());
    const auto& decode_failure =
        lumalink::json::test_support::assert_expected_error(decoded, lumalink::json::error_code::validation_failed);
    lumalink::json::test_support::assert_error_context_depth(decode_failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(decode_failure, 0U, lumalink::json::node_kind::any);
    lumalink::json::test_support::assert_error_context_entry(
        decode_failure,
        1U,
        lumalink::json::node_kind::field,
        "payload");
    lumalink::json::test_support::assert_error_context_entry(decode_failure, 2U, lumalink::json::node_kind::object);

    JsonDocument encoded;
    const auto encoded_result = lumalink::json::serialize<dynamic_payload_holder_spec>(
        dynamic_payload_holder{std::any{uint8_t{7}}},
        encoded);
    const auto& encode_failure = lumalink::json::test_support::assert_expected_error(
        encoded_result,
        lumalink::json::error_code::validation_failed);
    lumalink::json::test_support::assert_error_context_depth(encode_failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(encode_failure, 0U, lumalink::json::node_kind::any);
    lumalink::json::test_support::assert_error_context_entry(
        encode_failure,
        1U,
        lumalink::json::node_kind::field,
        "payload");
    lumalink::json::test_support::assert_error_context_entry(encode_failure, 2U, lumalink::json::node_kind::object);
}

void run_phase7_extensibility_tests() {
    RUN_TEST(test_ext_01_custom_decoder_specialization_supports_non_standard_targets);
    RUN_TEST(test_ext_01b_custom_decoder_parses_indexed_color_stop_lists);
    RUN_TEST(test_ext_02_custom_encoder_specialization_supports_non_standard_sources);
    RUN_TEST(test_ext_03_custom_enum_mapping_traits_support_project_specific_enums);
    RUN_TEST(test_ext_03a_enum_codec_lookup_helpers_resolve_mapped_entries);
    RUN_TEST(test_ext_03b_enum_codec_lookup_helpers_report_missing_entries);
    RUN_TEST(test_ext_04_custom_converter_failures_propagate_expected_errors);
    RUN_TEST(test_ext_05_codec_wrapped_spec_supports_std_any_decode);
    RUN_TEST(test_ext_06_codec_wrapped_spec_supports_std_any_encode);
    RUN_TEST(test_ext_07_codec_wrapped_spec_supports_second_tagged_structure);
    RUN_TEST(test_ext_08_codec_wrapped_spec_propagates_std_any_failures);
}