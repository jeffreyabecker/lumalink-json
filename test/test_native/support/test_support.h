#pragma once

#include <ArduinoJson.h>
#include <unity.h>

#include <cstddef>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <json.h>

namespace lumalink::json::test_support {

struct native_fixture {
    JsonDocument document{};

    void parse_or_fail(const std::string_view input) {
        document.clear();
        const auto parse_error = deserializeJson(document, input.data(), input.size());
        TEST_ASSERT_FALSE_MESSAGE(parse_error, parse_error.c_str());
    }

    JsonVariantConst root() const {
        return document.as<JsonVariantConst>();
    }

    std::string serialize_or_fail() {
        std::string output;
        const size_t bytes_written = serializeJson(document, output);
        TEST_ASSERT_GREATER_THAN_UINT32(0U, static_cast<uint32_t>(bytes_written));
        return output;
    }
};

inline error_code map_deserialization_error(const DeserializationError parse_error) {
    return detail::map_deserialization_error(parse_error);
}

template <typename T>
const T& assert_expected_success(const expected<T>& result) {
    TEST_ASSERT_TRUE(result.has_value());
    return *result;
}

inline void assert_expected_success(const expected_void& result) {
    TEST_ASSERT_TRUE(result.has_value());
}

template <typename T>
const error& assert_expected_error(const expected<T>& result, const error_code expected_code) {
    TEST_ASSERT_FALSE(result.has_value());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(expected_code), static_cast<int>(result.error().code));
    return result.error();
}

inline const error& assert_expected_error(const expected_void& result, const error_code expected_code) {
    TEST_ASSERT_FALSE(result.has_value());
    TEST_ASSERT_EQUAL_INT(static_cast<int>(expected_code), static_cast<int>(result.error().code));
    return result.error();
}

inline void assert_error_context_depth(const error& actual, const size_t expected_depth) {
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(expected_depth), static_cast<uint32_t>(actual.context.size));
}

inline void assert_error_context_entry(
    const error& actual,
    const size_t index,
    const node_kind expected_kind,
    const std::string_view expected_field_key = {},
    const std::string_view expected_logical_name = {},
    const size_t expected_index = std::numeric_limits<size_t>::max()) {
    TEST_ASSERT_TRUE(index < actual.context.size);

    const auto& entry = actual.context.entries[index];
    TEST_ASSERT_EQUAL_INT(static_cast<int>(expected_kind), static_cast<int>(entry.kind));
    TEST_ASSERT_EQUAL_UINT32(
        static_cast<uint32_t>(expected_field_key.size()),
        static_cast<uint32_t>(entry.field_key.size()));
    TEST_ASSERT_EQUAL_UINT32(
        static_cast<uint32_t>(expected_logical_name.size()),
        static_cast<uint32_t>(entry.logical_name.size()));

    if (expected_index == std::numeric_limits<size_t>::max()) {
        TEST_ASSERT_FALSE(entry.has_index);
    } else {
        TEST_ASSERT_TRUE(entry.has_index);
        TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(expected_index), static_cast<uint32_t>(entry.index));
    }

    if (!expected_field_key.empty()) {
        TEST_ASSERT_EQUAL_INT(
            0,
            std::memcmp(expected_field_key.data(), entry.field_key.data(), expected_field_key.size()));
    }

    if (!expected_logical_name.empty()) {
        TEST_ASSERT_EQUAL_INT(
            0,
            std::memcmp(expected_logical_name.data(), entry.logical_name.data(), expected_logical_name.size()));
    }
}

template <typename Model, typename EncodeFn, typename DecodeFn>
expected<Model> round_trip(const Model& model, EncodeFn&& encode_fn, DecodeFn&& decode_fn) {
    JsonDocument document;
    const auto encode_result = std::forward<EncodeFn>(encode_fn)(model, document);
    if (!encode_result.has_value()) {
        return std::unexpected(encode_result.error());
    }

    std::string serialized;
    if (serializeJson(document, serialized) == 0U) {
        return std::unexpected(error{error_code::invalid_input, {}, "round-trip serialize failed"});
    }

    JsonDocument reparsed;
    const auto parse_error = deserializeJson(reparsed, serialized);
    if (parse_error) {
        return std::unexpected(error{map_deserialization_error(parse_error), {}, parse_error.c_str()});
    }

    return std::forward<DecodeFn>(decode_fn)(reparsed.as<JsonVariantConst>());
}

} // namespace lumalink::json::test_support
