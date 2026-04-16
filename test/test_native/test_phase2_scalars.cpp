#include <ArduinoJson.h>
#include <unity.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

#include <LumaLinkJson.h>

#include "support/test_support.h"

namespace {

enum class test_mode : unsigned char {
    automatic,
    manual,
    disabled,
    orphan,
};

constexpr bool is_uppercase_token(const std::string_view value) {
    if (value.empty()) {
        return false;
    }

    for (const char character : value) {
        if (character < 'A' || character > 'Z') {
            return false;
        }
    }

    return true;
}

using uppercase_string_spec = lumalink::json::spec::string<lumalink::json::opts::pattern<is_uppercase_token>>;
using bounded_integer_spec = lumalink::json::spec::integer<lumalink::json::opts::min_max_value<10, 20>>;
using enum_mode_spec = lumalink::json::spec::enum_string<
    test_mode,
    lumalink::json::spec::enum_values<
        lumalink::json::spec::enum_value<test_mode::automatic, "auto">,
        lumalink::json::spec::enum_value<test_mode::manual, "manual">,
        lumalink::json::spec::enum_value<test_mode::disabled, "disabled">>>;

} // namespace

void test_scl_01_null_decode_success_from_json_null() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("null");

    const auto result = lumalink::json::deserialize<std::nullptr_t, lumalink::json::spec::null<>>(fixture.root());
    TEST_ASSERT_NULL(lumalink::json::test_support::assert_expected_success(result));
}

void test_scl_02_null_rejects_non_null_values() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("true");

    const auto result = lumalink::json::deserialize<std::nullptr_t, lumalink::json::spec::null<>>(fixture.root());
    lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
}

void test_scl_03_boolean_true_false_decode_and_encode() {
    lumalink::json::test_support::native_fixture true_fixture;
    true_fixture.parse_or_fail("true");
    TEST_ASSERT_TRUE(lumalink::json::test_support::assert_expected_success(
        lumalink::json::deserialize<bool, lumalink::json::spec::boolean<>>(true_fixture.root())));

    lumalink::json::test_support::native_fixture false_fixture;
    false_fixture.parse_or_fail("false");
    TEST_ASSERT_FALSE(lumalink::json::test_support::assert_expected_success(
        lumalink::json::deserialize<bool, lumalink::json::spec::boolean<>>(false_fixture.root())));

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<lumalink::json::spec::boolean<>>(true, encoded));
    TEST_ASSERT_TRUE(encoded.as<JsonVariantConst>().as<bool>());
}

void test_scl_04_boolean_wrong_type_failures() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("1");

    const auto result = lumalink::json::deserialize<bool, lumalink::json::spec::boolean<>>(fixture.root());
    lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
}

void test_scl_05_integer_signed_and_unsigned_target_types() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("42");

    TEST_ASSERT_EQUAL(
        42,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<int32_t, lumalink::json::spec::integer<>>(fixture.root())));
    TEST_ASSERT_EQUAL_UINT32(
        42U,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<uint32_t, lumalink::json::spec::integer<>>(fixture.root())));
}

void test_scl_06_integer_boundary_values_and_range_failures() {
    lumalink::json::test_support::native_fixture ok_fixture;
    ok_fixture.parse_or_fail("127");
    TEST_ASSERT_EQUAL_INT8(
        127,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<int8_t, lumalink::json::spec::integer<>>(ok_fixture.root())));

    lumalink::json::test_support::native_fixture overflow_fixture;
    overflow_fixture.parse_or_fail("128");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int8_t, lumalink::json::spec::integer<>>(overflow_fixture.root()),
        lumalink::json::error_code::value_out_of_range);

    lumalink::json::test_support::native_fixture negative_fixture;
    negative_fixture.parse_or_fail("-1");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<uint8_t, lumalink::json::spec::integer<>>(negative_fixture.root()),
        lumalink::json::error_code::value_out_of_range);
}

void test_scl_07_integer_min_max_value_option_behavior() {
    lumalink::json::test_support::native_fixture ok_fixture;
    ok_fixture.parse_or_fail("15");
    TEST_ASSERT_EQUAL(
        15,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<int, bounded_integer_spec>(ok_fixture.root())));

    lumalink::json::test_support::native_fixture bad_fixture;
    bad_fixture.parse_or_fail("25");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, bounded_integer_spec>(bad_fixture.root()),
        lumalink::json::error_code::value_out_of_range);
}

void test_scl_08_number_float_and_double_decode_and_encode() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("1.25");

    const auto decoded_float = lumalink::json::test_support::assert_expected_success(
        lumalink::json::deserialize<float, lumalink::json::spec::number<>>(fixture.root()));
    const auto decoded_double = lumalink::json::test_support::assert_expected_success(
        lumalink::json::deserialize<double, lumalink::json::spec::number<>>(fixture.root()));

    TEST_ASSERT_FLOAT_WITHIN(
        0.0001f,
        1.25f,
        decoded_float);
    TEST_ASSERT_TRUE(std::fabs(decoded_double - 1.25) < 0.0001);

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<lumalink::json::spec::number<>>(2.5, encoded));
    TEST_ASSERT_TRUE(std::fabs(encoded.as<JsonVariantConst>().as<double>() - 2.5) < 0.0001);
}

void test_scl_09_number_accepted_numeric_input_forms() {
    lumalink::json::test_support::native_fixture integer_fixture;
    integer_fixture.parse_or_fail("7");
    TEST_ASSERT_FLOAT_WITHIN(
        0.0001f,
        7.0f,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<float, lumalink::json::spec::number<>>(integer_fixture.root())));

    lumalink::json::test_support::native_fixture string_fixture;
    string_fixture.parse_or_fail(R"("7")");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<float, lumalink::json::spec::number<>>(string_fixture.root()),
        lumalink::json::error_code::unexpected_type);
}

void test_scl_10_string_empty_non_empty_and_wrong_type_values() {
    lumalink::json::test_support::native_fixture empty_fixture;
    empty_fixture.parse_or_fail(R"("")");
    TEST_ASSERT_EQUAL_STRING(
        "",
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<std::string, lumalink::json::spec::string<>>(empty_fixture.root())).c_str());

    lumalink::json::test_support::native_fixture value_fixture;
    value_fixture.parse_or_fail(R"("auto")");
    TEST_ASSERT_EQUAL_STRING(
        "auto",
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<std::string, lumalink::json::spec::string<>>(value_fixture.root())).c_str());

    lumalink::json::test_support::native_fixture bad_fixture;
    bad_fixture.parse_or_fail("5");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<std::string, lumalink::json::spec::string<>>(bad_fixture.root()),
        lumalink::json::error_code::unexpected_type);
}

void test_scl_11_string_lifetime_expectations_for_supported_targets() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"("AUTO")");

    const char* raw = fixture.root().as<const char*>();
    const auto borrowed_view = lumalink::json::test_support::assert_expected_success(
        lumalink::json::deserialize<std::string_view, lumalink::json::spec::string<>>(fixture.root()));
    const auto borrowed_pointer = lumalink::json::test_support::assert_expected_success(
        lumalink::json::deserialize<const char*, lumalink::json::spec::string<>>(fixture.root()));
    const auto owned_string_result = lumalink::json::deserialize<std::string, lumalink::json::spec::string<>>(fixture.root());
    const std::string owned_string = lumalink::json::test_support::assert_expected_success(owned_string_result);

    TEST_ASSERT_EQUAL_PTR(raw, borrowed_pointer);
    TEST_ASSERT_EQUAL_PTR(raw, borrowed_view.data());

    fixture.document.clear();
    TEST_ASSERT_EQUAL_STRING("AUTO", owned_string.c_str());
}

void test_scl_12_string_pattern_option_behavior() {
    lumalink::json::test_support::native_fixture ok_fixture;
    ok_fixture.parse_or_fail(R"("AUTO")");
    TEST_ASSERT_EQUAL_STRING(
        "AUTO",
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<std::string, uppercase_string_spec>(ok_fixture.root())).c_str());

    lumalink::json::test_support::native_fixture bad_fixture;
    bad_fixture.parse_or_fail(R"("Auto")");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<std::string, uppercase_string_spec>(bad_fixture.root()),
        lumalink::json::error_code::pattern_mismatch);

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::serialize<uppercase_string_spec>(std::string("Auto"), encoded),
        lumalink::json::error_code::pattern_mismatch);
}

void test_scl_13_enum_string_successful_decode_of_every_mapped_token() {
    lumalink::json::test_support::native_fixture auto_fixture;
    auto_fixture.parse_or_fail(R"("auto")");
    TEST_ASSERT_EQUAL(
        static_cast<int>(test_mode::automatic),
        static_cast<int>(lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<test_mode, enum_mode_spec>(auto_fixture.root()))));

    lumalink::json::test_support::native_fixture manual_fixture;
    manual_fixture.parse_or_fail(R"("manual")");
    TEST_ASSERT_EQUAL(
        static_cast<int>(test_mode::manual),
        static_cast<int>(lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<test_mode, enum_mode_spec>(manual_fixture.root()))));

    lumalink::json::test_support::native_fixture disabled_fixture;
    disabled_fixture.parse_or_fail(R"("disabled")");
    TEST_ASSERT_EQUAL(
        static_cast<int>(test_mode::disabled),
        static_cast<int>(lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<test_mode, enum_mode_spec>(disabled_fixture.root()))));
}

void test_scl_14_enum_string_successful_encode_of_every_mapped_value() {
    JsonDocument encoded;

    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<enum_mode_spec>(test_mode::automatic, encoded));
    TEST_ASSERT_EQUAL_STRING("auto", encoded.as<const char*>());

    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<enum_mode_spec>(test_mode::manual, encoded));
    TEST_ASSERT_EQUAL_STRING("manual", encoded.as<const char*>());

    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<enum_mode_spec>(test_mode::disabled, encoded));
    TEST_ASSERT_EQUAL_STRING("disabled", encoded.as<const char*>());
}

void test_scl_15_enum_string_unknown_on_unknown_input_tokens() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"("bogus")");

    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<test_mode, enum_mode_spec>(fixture.root()),
        lumalink::json::error_code::enum_string_unknown);
}

void test_scl_16_enum_value_unmapped_on_unmapped_encode_values() {
    JsonDocument encoded;

    lumalink::json::test_support::assert_expected_error(
        lumalink::json::serialize<enum_mode_spec>(test_mode::orphan, encoded),
        lumalink::json::error_code::enum_value_unmapped);
}

void test_scl_17_any_pass_through_decode_and_encode_behavior() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"mode":"auto","count":2})");

    const auto decoded = lumalink::json::deserialize<JsonVariantConst, lumalink::json::spec::any<>>(fixture.root());
    const auto value = lumalink::json::test_support::assert_expected_success(decoded);
    TEST_ASSERT_EQUAL_STRING("auto", value["mode"]);
    TEST_ASSERT_EQUAL(2, value["count"].as<int>());

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<lumalink::json::spec::any<>>(value, encoded));
    TEST_ASSERT_EQUAL_STRING("auto", encoded["mode"]);
    TEST_ASSERT_EQUAL(2, encoded["count"].as<int>());
}

void run_phase2_scalar_tests() {
    RUN_TEST(test_scl_01_null_decode_success_from_json_null);
    RUN_TEST(test_scl_02_null_rejects_non_null_values);
    RUN_TEST(test_scl_03_boolean_true_false_decode_and_encode);
    RUN_TEST(test_scl_04_boolean_wrong_type_failures);
    RUN_TEST(test_scl_05_integer_signed_and_unsigned_target_types);
    RUN_TEST(test_scl_06_integer_boundary_values_and_range_failures);
    RUN_TEST(test_scl_07_integer_min_max_value_option_behavior);
    RUN_TEST(test_scl_08_number_float_and_double_decode_and_encode);
    RUN_TEST(test_scl_09_number_accepted_numeric_input_forms);
    RUN_TEST(test_scl_10_string_empty_non_empty_and_wrong_type_values);
    RUN_TEST(test_scl_11_string_lifetime_expectations_for_supported_targets);
    RUN_TEST(test_scl_12_string_pattern_option_behavior);
    RUN_TEST(test_scl_13_enum_string_successful_decode_of_every_mapped_token);
    RUN_TEST(test_scl_14_enum_string_successful_encode_of_every_mapped_value);
    RUN_TEST(test_scl_15_enum_string_unknown_on_unknown_input_tokens);
    RUN_TEST(test_scl_16_enum_value_unmapped_on_unmapped_encode_values);
    RUN_TEST(test_scl_17_any_pass_through_decode_and_encode_behavior);
}
