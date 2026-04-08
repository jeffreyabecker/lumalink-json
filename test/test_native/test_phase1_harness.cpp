#include <ArduinoJson.h>
#include <unity.h>

#include <string>

#include <LumaLinkJson.h>

#include "support/test_support.h"

void test_tst_01_native_fixture_parses_and_serializes_json() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"mode":"auto","enabled":true})");

    TEST_ASSERT_EQUAL_STRING("auto", fixture.root()["mode"]);
    TEST_ASSERT_TRUE(fixture.root()["enabled"].as<bool>());

    const std::string serialized = fixture.serialize_or_fail();
    TEST_ASSERT_NOT_EQUAL(0, static_cast<int>(serialized.size()));
}

void test_tst_02_expected_assertion_helpers_cover_success_and_failure() {
    const lumalink::json::expected<int> success = 42;
    TEST_ASSERT_EQUAL(42, lumalink::json::test_support::assert_expected_success(success));

    const auto failure = lumalink::json::not_implemented();
    lumalink::json::test_support::assert_expected_error(
        failure,
        lumalink::json::error_code::not_implemented);
}

void test_tst_03_error_context_helpers_cover_kind_key_and_depth() {
    auto failure = lumalink::json::error{
        lumalink::json::error_code::missing_field,
        {},
        "missing field"
    };

    failure = lumalink::json::with_context(
        failure,
        {
            lumalink::json::node_kind::object,
            "root-config",
            {}
        });
    failure = lumalink::json::with_context(
        failure,
        {
            lumalink::json::node_kind::field,
            "mode-name",
            "mode"
        });

    lumalink::json::test_support::assert_error_context_depth(failure, 2U);
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        0U,
        lumalink::json::node_kind::object,
        {},
        "root-config");
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        1U,
        lumalink::json::node_kind::field,
        "mode",
        "mode-name");
}

void test_tst_04_round_trip_helper_reparses_encoded_json() {
    const auto result = lumalink::json::test_support::round_trip<std::string>(
        "auto",
        [](const std::string& value, JsonDocument& document) -> lumalink::json::expected_void {
            document["mode"] = value;
            return {};
        },
        [](const JsonVariantConst root) -> lumalink::json::expected<std::string> {
            const char* value = root["mode"];
            if (value == nullptr) {
                return std::unexpected(lumalink::json::error{
                    lumalink::json::error_code::missing_field,
                    {},
                    "mode field missing"
                });
            }

            return std::string(value);
        });

    TEST_ASSERT_EQUAL_STRING(
        "auto",
        lumalink::json::test_support::assert_expected_success(result).c_str());
}

void test_tst_05_not_implemented_error_alias_smoke() {
    const auto result = lumalink::json::not_implemented();

    TEST_ASSERT_FALSE(result.has_value());
    TEST_ASSERT_EQUAL(
        static_cast<int>(lumalink::json::error_code::not_implemented),
        static_cast<int>(result.error().code));
}

void run_phase1_harness_tests() {
    RUN_TEST(test_tst_01_native_fixture_parses_and_serializes_json);
    RUN_TEST(test_tst_02_expected_assertion_helpers_cover_success_and_failure);
    RUN_TEST(test_tst_03_error_context_helpers_cover_kind_key_and_depth);
    RUN_TEST(test_tst_04_round_trip_helper_reparses_encoded_json);
    RUN_TEST(test_tst_05_not_implemented_error_alias_smoke);
}
