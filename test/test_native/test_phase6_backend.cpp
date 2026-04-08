#include <ArduinoJson.h>
#include <unity.h>

#include <string>
#include <string_view>

#include <json.h>

#include "support/test_support.h"

namespace {

struct parse_mode_model {
    int id;
    bool enabled;
};

using parse_mode_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"enabled", lumalink::json::spec::boolean<>>>;

class failing_allocator final : public ArduinoJson::Allocator {
public:
    void* allocate(const size_t size) override {
        (void)size;
        return nullptr;
    }

    void deallocate(void* ptr) override {
        (void)ptr;
    }

    void* reallocate(void* ptr, const size_t new_size) override {
        (void)ptr;
        (void)new_size;
        return nullptr;
    }
};

} // namespace

void test_bck_01_deserialize_accepts_json_variant_const() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"id":7,"enabled":true})");

    const auto result = lumalink::json::deserialize<parse_mode_model, parse_mode_spec>(fixture.root());
    const auto& value = lumalink::json::test_support::assert_expected_success(result);
    TEST_ASSERT_EQUAL(7, value.id);
    TEST_ASSERT_TRUE(value.enabled);
}

void test_bck_02_deserialize_accepts_raw_json_input() {
    const auto result = lumalink::json::deserialize<parse_mode_model, parse_mode_spec>(
        std::string_view{R"({"id":9,"enabled":false})"});
    const auto& value = lumalink::json::test_support::assert_expected_success(result);
    TEST_ASSERT_EQUAL(9, value.id);
    TEST_ASSERT_FALSE(value.enabled);
}

void test_bck_03_empty_input_maps_to_empty_input() {
    JsonDocument document;
    const auto result = lumalink::json::deserialize<parse_mode_model, parse_mode_spec>(std::string_view{}, document);
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::empty_input);
    TEST_ASSERT_EQUAL(
        static_cast<int>(DeserializationError::Code::EmptyInput),
        failure.backend_status);
}

void test_bck_04_incomplete_json_maps_to_incomplete_input() {
    JsonDocument document;
    const auto result = lumalink::json::deserialize<parse_mode_model, parse_mode_spec>(
        std::string_view{"{\"id\":7"},
        document);
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::incomplete_input);
    TEST_ASSERT_EQUAL(
        static_cast<int>(DeserializationError::Code::IncompleteInput),
        failure.backend_status);
}

void test_bck_05_malformed_json_maps_to_invalid_input() {
    JsonDocument document;
    const auto result = lumalink::json::deserialize<parse_mode_model, parse_mode_spec>(
        std::string_view{"{]"},
        document);
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::invalid_input);
    TEST_ASSERT_EQUAL(
        static_cast<int>(DeserializationError::Code::InvalidInput),
        failure.backend_status);
}

void test_bck_06_backend_memory_exhaustion_maps_to_no_memory() {
    failing_allocator allocator;
    JsonDocument document{&allocator};

    const auto result = lumalink::json::deserialize<parse_mode_model, parse_mode_spec>(
        std::string_view{R"({"id":11,"enabled":true})"},
        document);
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::no_memory);
    TEST_ASSERT_EQUAL(
        static_cast<int>(DeserializationError::Code::NoMemory),
        failure.backend_status);
}

void test_bck_07_excessive_nesting_maps_to_too_deep() {
    JsonDocument document;
    const auto result = lumalink::json::deserialize<int, lumalink::json::spec::integer<>>(
        std::string_view{"[[[[1]]]]"},
        document,
        lumalink::json::decode_options{lumalink::json::context_policy::full, 2U});
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::too_deep);
    TEST_ASSERT_EQUAL(
        static_cast<int>(DeserializationError::Code::TooDeep),
        failure.backend_status);
}

void run_phase6_backend_tests() {
    RUN_TEST(test_bck_01_deserialize_accepts_json_variant_const);
    RUN_TEST(test_bck_02_deserialize_accepts_raw_json_input);
    RUN_TEST(test_bck_03_empty_input_maps_to_empty_input);
    RUN_TEST(test_bck_04_incomplete_json_maps_to_incomplete_input);
    RUN_TEST(test_bck_05_malformed_json_maps_to_invalid_input);
    RUN_TEST(test_bck_06_backend_memory_exhaustion_maps_to_no_memory);
    RUN_TEST(test_bck_07_excessive_nesting_maps_to_too_deep);
}