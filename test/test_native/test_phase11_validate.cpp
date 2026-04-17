#include <ArduinoJson.h>
#include <unity.h>

#include <optional>
#include <string>
#include <string_view>

#include <LumaLinkJson.h>

#include "support/test_support.h"

namespace {

struct validate_point {
    int x;
    int y;
};

struct validate_labeled {
    std::string name;
    std::optional<int> value;
};

class non_aggregate_point {
public:
    non_aggregate_point() : x(0), y(0) {}
    int x;
    int y;
};

struct non_aggregate_point_binding {
    using model_type = non_aggregate_point;
    static constexpr auto members = std::make_tuple(
        &non_aggregate_point::x, &non_aggregate_point::y);
};

constexpr bool validate_non_negative(const int value) {
    return value >= 0;
}

using point_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"x", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"y", lumalink::json::spec::integer<>>>;

using bounded_x_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"x", lumalink::json::spec::integer<lumalink::json::opts::validator_func<validate_non_negative>>>,
    lumalink::json::spec::field<"y", lumalink::json::spec::integer<>>>;

using labeled_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"name", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"value", lumalink::json::spec::optional<lumalink::json::spec::integer<>>>>;

} // namespace

namespace {

void test_val_01_validate_cpp_value_succeeds_for_valid_object() {
    validate_point pt{3, 7};
    const auto result = lumalink::json::validate<point_spec, validate_point>(pt);
    lumalink::json::test_support::assert_expected_success(result);
}

void test_val_02_validate_cpp_value_fails_for_field_constraint_violation() {
    validate_point pt{-1, 7};
    const auto result = lumalink::json::validate<bounded_x_spec, validate_point>(pt);
    lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::validation_failed);
}

void test_val_03_validate_json_string_succeeds_for_valid_input() {
    const auto result = lumalink::json::validate<validate_point, point_spec>(
        std::string_view{R"({"x":1,"y":2})"});
    lumalink::json::test_support::assert_expected_success(result);
}

void test_val_04_validate_json_string_fails_for_type_mismatch() {
    // "x" is a string instead of an integer.
    const auto result = lumalink::json::validate<validate_point, point_spec>(
        std::string_view{R"({"x":"bad","y":2})"});
    lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
}

void test_val_05_validate_json_string_fails_for_missing_required_field() {
    const auto result = lumalink::json::validate<validate_point, point_spec>(
        std::string_view{R"({"x":5})"});
    lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::missing_field);
}

void test_val_06_validate_json_variant_succeeds() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"x":10,"y":20})");
    const auto result = lumalink::json::validate<validate_point, point_spec>(fixture.root());
    lumalink::json::test_support::assert_expected_success(result);
}

void test_val_07_validate_json_document_succeeds() {
    JsonDocument document;
    deserializeJson(document, R"({"x":0,"y":0})");
    const auto result = lumalink::json::validate<validate_point, point_spec>(document);
    lumalink::json::test_support::assert_expected_success(result);
}

void test_val_08_validate_optional_field_present_succeeds() {
    const auto result = lumalink::json::validate<validate_labeled, labeled_spec>(
        std::string_view{R"({"name":"hello","value":42})"});
    lumalink::json::test_support::assert_expected_success(result);
}

void test_val_09_validate_optional_field_absent_succeeds() {
    const auto result = lumalink::json::validate<validate_labeled, labeled_spec>(
        std::string_view{R"({"name":"hello"})"});
    lumalink::json::test_support::assert_expected_success(result);
}

void test_val_10_validate_with_explicit_binding_succeeds() {
    non_aggregate_point pt;
    pt.x = 5;
    pt.y = 9;

    // Encode-path validate with explicit binding.
    const auto encode_result =
        lumalink::json::validate<point_spec, non_aggregate_point, non_aggregate_point_binding>(pt);
    lumalink::json::test_support::assert_expected_success(encode_result);

    // Decode-path validate with explicit binding.
    const auto decode_result =
        lumalink::json::validate<non_aggregate_point, point_spec, non_aggregate_point_binding>(
            std::string_view{R"({"x":5,"y":9})"});
    lumalink::json::test_support::assert_expected_success(decode_result);
}

} // namespace

void run_phase11_validate_tests() {
    RUN_TEST(test_val_01_validate_cpp_value_succeeds_for_valid_object);
    RUN_TEST(test_val_02_validate_cpp_value_fails_for_field_constraint_violation);
    RUN_TEST(test_val_03_validate_json_string_succeeds_for_valid_input);
    RUN_TEST(test_val_04_validate_json_string_fails_for_type_mismatch);
    RUN_TEST(test_val_05_validate_json_string_fails_for_missing_required_field);
    RUN_TEST(test_val_06_validate_json_variant_succeeds);
    RUN_TEST(test_val_07_validate_json_document_succeeds);
    RUN_TEST(test_val_08_validate_optional_field_present_succeeds);
    RUN_TEST(test_val_09_validate_optional_field_absent_succeeds);
    RUN_TEST(test_val_10_validate_with_explicit_binding_succeeds);
}
