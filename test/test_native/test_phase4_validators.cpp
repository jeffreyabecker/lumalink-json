#include <ArduinoJson.h>
#include <unity.h>

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include <LumaLinkJson.h>

#include "support/test_support.h"

namespace {

int g_bool_validator_calls = 0;
int g_expected_validator_calls = 0;
int g_ordered_validator_calls = 0;
int g_encode_validator_calls = 0;

bool validate_even_integer(const int value) {
    ++g_bool_validator_calls;
    return (value % 2) == 0;
}

std::expected<void, lumalink::json::error_code> validate_nonzero_integer(const int value) {
    ++g_expected_validator_calls;
    if (value != 0) {
        return {};
    }

    return std::unexpected(lumalink::json::error_code::validation_failed);
}

bool validate_small_even_integer(const int value) {
    ++g_ordered_validator_calls;
    return (value % 2) == 0 && value <= 20;
}

bool validate_encodable_integer(const int value) {
    ++g_encode_validator_calls;
    return value <= 100;
}

bool is_uppercase_token_phase4(const std::string_view value) {
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

struct named_speed_model {
    int speed;
};

using bool_validator_spec =
    lumalink::json::spec::integer<lumalink::json::opts::validator_func<validate_even_integer>>;
using expected_validator_spec =
    lumalink::json::spec::integer<lumalink::json::opts::validator_func<validate_nonzero_integer>>;
using ordered_validator_spec = lumalink::json::spec::integer<
    lumalink::json::opts::min_max_value<10, 20>,
    lumalink::json::opts::validator_func<validate_small_even_integer>>;
using encode_validator_spec =
    lumalink::json::spec::integer<lumalink::json::opts::validator_func<validate_encodable_integer>>;
using named_speed_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<
        "speed",
        lumalink::json::spec::integer<lumalink::json::opts::name<"motor_speed">>>>;
using bounded_values_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::min_max_elements<1, 3>>;
using bounded_integer_spec = lumalink::json::spec::integer<lumalink::json::opts::min_max_value<10, 20>>;
using uppercase_string_spec_phase4 =
    lumalink::json::spec::string<lumalink::json::opts::pattern<is_uppercase_token_phase4>>;

} // namespace

void test_val_01_boolean_return_validator_success() {
    g_bool_validator_calls = 0;

    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("12");

    TEST_ASSERT_EQUAL(
        12,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<int, bool_validator_spec>(fixture.root())));
    TEST_ASSERT_EQUAL(1, g_bool_validator_calls);
}

void test_val_02_boolean_return_validator_failure() {
    g_bool_validator_calls = 0;

    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("11");

    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, bool_validator_spec>(fixture.root()),
        lumalink::json::error_code::validation_failed);
    TEST_ASSERT_EQUAL(1, g_bool_validator_calls);
}

void test_val_03_expected_return_validator_success() {
    g_expected_validator_calls = 0;

    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("5");

    TEST_ASSERT_EQUAL(
        5,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<int, expected_validator_spec>(fixture.root())));
    TEST_ASSERT_EQUAL(1, g_expected_validator_calls);
}

void test_val_04_expected_return_validator_failure() {
    g_expected_validator_calls = 0;

    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("0");

    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, expected_validator_spec>(fixture.root()),
        lumalink::json::error_code::validation_failed);
    TEST_ASSERT_EQUAL(1, g_expected_validator_calls);
}

void test_val_05_validators_run_after_successful_base_decode() {
    g_ordered_validator_calls = 0;

    lumalink::json::test_support::native_fixture type_fixture;
    type_fixture.parse_or_fail(R"("bad")");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, ordered_validator_spec>(type_fixture.root()),
        lumalink::json::error_code::unexpected_type);
    TEST_ASSERT_EQUAL(0, g_ordered_validator_calls);

    lumalink::json::test_support::native_fixture bounds_fixture;
    bounds_fixture.parse_or_fail("9");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, ordered_validator_spec>(bounds_fixture.root()),
        lumalink::json::error_code::value_out_of_range);
    TEST_ASSERT_EQUAL(0, g_ordered_validator_calls);

    lumalink::json::test_support::native_fixture success_fixture;
    success_fixture.parse_or_fail("12");
    TEST_ASSERT_EQUAL(
        12,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<int, ordered_validator_spec>(success_fixture.root())));
    TEST_ASSERT_EQUAL(1, g_ordered_validator_calls);
}

void test_val_06_validators_run_during_encode() {
    g_encode_validator_calls = 0;

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<encode_validator_spec>(42, encoded));
    TEST_ASSERT_EQUAL(1, g_encode_validator_calls);

    lumalink::json::test_support::assert_expected_error(
        lumalink::json::serialize<encode_validator_spec>(101, encoded),
        lumalink::json::error_code::validation_failed);
    TEST_ASSERT_EQUAL(2, g_encode_validator_calls);
}

void test_val_07_name_option_appears_in_error_context() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"speed":"fast"})");

    const auto result = lumalink::json::deserialize<named_speed_model, named_speed_spec>(fixture.root());
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        0U,
        lumalink::json::node_kind::integer,
        {},
        "motor_speed");
    lumalink::json::test_support::assert_error_context_entry(failure, 1U, lumalink::json::node_kind::field, "speed");
    lumalink::json::test_support::assert_error_context_entry(failure, 2U, lumalink::json::node_kind::object);
}

void test_val_08_min_max_elements_lower_and_upper_bounds() {
    lumalink::json::test_support::native_fixture lower_fixture;
    lower_fixture.parse_or_fail("[4]");
    const auto lower_result = lumalink::json::deserialize<std::vector<int>, bounded_values_spec>(lower_fixture.root());
    const auto& lower_values = lumalink::json::test_support::assert_expected_success(lower_result);
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<uint32_t>(lower_values.size()));

    lumalink::json::test_support::native_fixture upper_fixture;
    upper_fixture.parse_or_fail("[4,5,6]");
    const auto upper_result = lumalink::json::deserialize<std::vector<int>, bounded_values_spec>(upper_fixture.root());
    const auto& upper_values = lumalink::json::test_support::assert_expected_success(upper_result);
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(upper_values.size()));

    lumalink::json::test_support::native_fixture below_fixture;
    below_fixture.parse_or_fail("[]");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<std::vector<int>, bounded_values_spec>(below_fixture.root()),
        lumalink::json::error_code::value_out_of_range);

    lumalink::json::test_support::native_fixture above_fixture;
    above_fixture.parse_or_fail("[4,5,6,7]");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<std::vector<int>, bounded_values_spec>(above_fixture.root()),
        lumalink::json::error_code::value_out_of_range);

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<bounded_values_spec>(std::vector<int>{1}, encoded));
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<bounded_values_spec>(std::vector<int>{1, 2, 3}, encoded));
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::serialize<bounded_values_spec>(std::vector<int>{1, 2, 3, 4}, encoded),
        lumalink::json::error_code::value_out_of_range);
}

void test_val_09_min_max_value_lower_and_upper_bounds() {
    lumalink::json::test_support::native_fixture lower_fixture;
    lower_fixture.parse_or_fail("10");
    TEST_ASSERT_EQUAL(
        10,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<int, bounded_integer_spec>(lower_fixture.root())));

    lumalink::json::test_support::native_fixture upper_fixture;
    upper_fixture.parse_or_fail("20");
    TEST_ASSERT_EQUAL(
        20,
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<int, bounded_integer_spec>(upper_fixture.root())));

    lumalink::json::test_support::native_fixture below_fixture;
    below_fixture.parse_or_fail("9");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, bounded_integer_spec>(below_fixture.root()),
        lumalink::json::error_code::value_out_of_range);

    lumalink::json::test_support::native_fixture above_fixture;
    above_fixture.parse_or_fail("21");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, bounded_integer_spec>(above_fixture.root()),
        lumalink::json::error_code::value_out_of_range);

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<bounded_integer_spec>(10, encoded));
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<bounded_integer_spec>(20, encoded));
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::serialize<bounded_integer_spec>(21, encoded),
        lumalink::json::error_code::value_out_of_range);
}

void test_val_10_pattern_accepts_and_rejects_values() {
    lumalink::json::test_support::native_fixture accepted_fixture;
    accepted_fixture.parse_or_fail(R"("AUTO")");
    TEST_ASSERT_EQUAL_STRING(
        "AUTO",
        lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<std::string, uppercase_string_spec_phase4>(accepted_fixture.root())).c_str());

    lumalink::json::test_support::native_fixture rejected_fixture;
    rejected_fixture.parse_or_fail(R"("Auto")");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<std::string, uppercase_string_spec_phase4>(rejected_fixture.root()),
        lumalink::json::error_code::pattern_mismatch);

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<uppercase_string_spec_phase4>(std::string("READY"), encoded));
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::serialize<uppercase_string_spec_phase4>(std::string("Ready"), encoded),
        lumalink::json::error_code::pattern_mismatch);
}

void run_phase4_validator_tests() {
    RUN_TEST(test_val_01_boolean_return_validator_success);
    RUN_TEST(test_val_02_boolean_return_validator_failure);
    RUN_TEST(test_val_03_expected_return_validator_success);
    RUN_TEST(test_val_04_expected_return_validator_failure);
    RUN_TEST(test_val_05_validators_run_after_successful_base_decode);
    RUN_TEST(test_val_06_validators_run_during_encode);
    RUN_TEST(test_val_07_name_option_appears_in_error_context);
    RUN_TEST(test_val_08_min_max_elements_lower_and_upper_bounds);
    RUN_TEST(test_val_09_min_max_value_lower_and_upper_bounds);
    RUN_TEST(test_val_10_pattern_accepts_and_rejects_values);
}