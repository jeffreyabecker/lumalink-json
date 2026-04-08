#include <ArduinoJson.h>
#include <unity.h>

#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <json.h>

#include "support/test_support.h"

namespace {

struct device_settings {
    int id;
    std::string label;
    bool enabled;
};

struct profile_state {
    int level;
    bool active;
};

struct profile_envelope {
    profile_state profile;
};

struct optional_counter {
    std::optional<int> count;
};

using settings_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"display_name", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"enabled", lumalink::json::spec::boolean<>>>;

using profile_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"level", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"active", lumalink::json::spec::boolean<>>>;

using profile_envelope_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"profile", profile_spec>>;

using optional_counter_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"count", lumalink::json::spec::optional<lumalink::json::spec::integer<>>>>;

using int_list_spec = lumalink::json::spec::array_of<lumalink::json::spec::integer<>>;
using bounded_int_list_spec =
    lumalink::json::spec::array_of<lumalink::json::spec::integer<>, lumalink::json::opts::min_max_elements<1, 3>>;
using triple_spec = lumalink::json::spec::tuple<
    lumalink::json::spec::integer<>,
    lumalink::json::spec::string<>,
    lumalink::json::spec::boolean<>>;
using int_or_bool_spec =
    lumalink::json::spec::one_of<lumalink::json::spec::integer<>, lumalink::json::spec::boolean<>>;
using int_or_number_spec =
    lumalink::json::spec::one_of<lumalink::json::spec::integer<>, lumalink::json::spec::number<>>;

} // namespace

void test_cmp_01_field_and_object_required_field_decode_success() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"id":7,"display_name":"AUTO","enabled":true})");

    const auto decoded = lumalink::json::deserialize<device_settings, settings_spec>(fixture.root());
    const auto& value = lumalink::json::test_support::assert_expected_success(decoded);
    TEST_ASSERT_EQUAL(7, value.id);
    TEST_ASSERT_EQUAL_STRING("AUTO", value.label.c_str());
    TEST_ASSERT_TRUE(value.enabled);

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<settings_spec>(device_settings{7, "AUTO", true}, encoded));
    std::string serialized;
    serializeJson(encoded, serialized);
    TEST_ASSERT_EQUAL_STRING("{\"id\":7,\"display_name\":\"AUTO\",\"enabled\":true}", serialized.c_str());
}

void test_cmp_02_field_and_object_missing_field_failures() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"id":7,"display_name":"AUTO"})");

    const auto result = lumalink::json::deserialize<device_settings, settings_spec>(fixture.root());
    const auto& failure = lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::missing_field);
    lumalink::json::test_support::assert_error_context_depth(failure, 2U);
    lumalink::json::test_support::assert_error_context_entry(failure, 0U, lumalink::json::node_kind::field, "enabled");
    lumalink::json::test_support::assert_error_context_entry(failure, 1U, lumalink::json::node_kind::object);
}

void test_cmp_03_field_and_object_nested_field_context_propagation() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"profile":{"level":"bad","active":true}})");

    const auto result = lumalink::json::deserialize<profile_envelope, profile_envelope_spec>(fixture.root());
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 5U);
    lumalink::json::test_support::assert_error_context_entry(failure, 0U, lumalink::json::node_kind::integer);
    lumalink::json::test_support::assert_error_context_entry(failure, 1U, lumalink::json::node_kind::field, "level");
    lumalink::json::test_support::assert_error_context_entry(failure, 2U, lumalink::json::node_kind::object);
    lumalink::json::test_support::assert_error_context_entry(failure, 3U, lumalink::json::node_kind::field, "profile");
    lumalink::json::test_support::assert_error_context_entry(failure, 4U, lumalink::json::node_kind::object);
}

void test_cmp_04_object_declaration_order_traversal_is_deterministic() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"id":"bad","display_name":9,"enabled":true})");

    const auto result = lumalink::json::deserialize<device_settings, settings_spec>(fixture.root());
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_entry(failure, 1U, lumalink::json::node_kind::field, "id");
}

void test_cmp_05_optional_present_values() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"count":12})");

    const auto decoded = lumalink::json::deserialize<optional_counter, optional_counter_spec>(fixture.root());
    const auto& value = lumalink::json::test_support::assert_expected_success(decoded);
    TEST_ASSERT_TRUE(value.count.has_value());
    TEST_ASSERT_EQUAL(12, *value.count);
}

void test_cmp_06_optional_missing_fields() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({})");

    const auto decoded = lumalink::json::deserialize<optional_counter, optional_counter_spec>(fixture.root());
    const auto& value = lumalink::json::test_support::assert_expected_success(decoded);
    TEST_ASSERT_FALSE(value.count.has_value());
}

void test_cmp_07_optional_explicit_json_null_values() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"count":null})");

    const auto decoded = lumalink::json::deserialize<optional_counter, optional_counter_spec>(fixture.root());
    const auto& value = lumalink::json::test_support::assert_expected_success(decoded);
    TEST_ASSERT_FALSE(value.count.has_value());
}

void test_cmp_08_array_of_homogeneous_container_decode_and_encode() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("[1,2,3]");

    const auto decoded = lumalink::json::deserialize<std::vector<int>, int_list_spec>(fixture.root());
    const auto& values = lumalink::json::test_support::assert_expected_success(decoded);
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(values.size()));
    TEST_ASSERT_EQUAL(1, values[0]);
    TEST_ASSERT_EQUAL(2, values[1]);
    TEST_ASSERT_EQUAL(3, values[2]);

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<int_list_spec>(std::vector<int>{4, 5}, encoded));
    TEST_ASSERT_EQUAL(4, encoded[0].as<int>());
    TEST_ASSERT_EQUAL(5, encoded[1].as<int>());
}

void test_cmp_09_array_of_per_element_type_failures_have_index_context() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("[1,true,3]");

    const auto result = lumalink::json::deserialize<std::vector<int>, int_list_spec>(fixture.root());
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(failure, 0U, lumalink::json::node_kind::integer);
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        1U,
        lumalink::json::node_kind::array_of,
        {},
        {},
        1U);
    lumalink::json::test_support::assert_error_context_entry(failure, 2U, lumalink::json::node_kind::array_of);
}

void test_cmp_10_array_of_min_max_elements_behavior() {
    lumalink::json::test_support::native_fixture ok_fixture;
    ok_fixture.parse_or_fail("[1,2,3]");
    TEST_ASSERT_EQUAL_UINT32(
        3U,
        static_cast<uint32_t>(lumalink::json::test_support::assert_expected_success(
            lumalink::json::deserialize<std::vector<int>, bounded_int_list_spec>(ok_fixture.root()))
                              .size()));

    lumalink::json::test_support::native_fixture bad_fixture;
    bad_fixture.parse_or_fail("[]");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<std::vector<int>, bounded_int_list_spec>(bad_fixture.root()),
        lumalink::json::error_code::value_out_of_range);
}

void test_cmp_11_tuple_exact_positional_decode_and_encode() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"([3,"AUTO",true])");

    const auto decoded = lumalink::json::deserialize<std::tuple<int, std::string, bool>, triple_spec>(fixture.root());
    const auto& value = lumalink::json::test_support::assert_expected_success(decoded);
    TEST_ASSERT_EQUAL(3, std::get<0>(value));
    TEST_ASSERT_EQUAL_STRING("AUTO", std::get<1>(value).c_str());
    TEST_ASSERT_TRUE(std::get<2>(value));

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<triple_spec>(std::make_tuple(9, std::string("MANUAL"), false), encoded));
    TEST_ASSERT_EQUAL(9, encoded[0].as<int>());
    TEST_ASSERT_EQUAL_STRING("MANUAL", encoded[1]);
    TEST_ASSERT_FALSE(encoded[2].as<bool>());
}

void test_cmp_12_tuple_array_size_mismatch_failures() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("[1,true]");

    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<std::tuple<int, std::string, bool>, triple_spec>(fixture.root()),
        lumalink::json::error_code::array_size_mismatch);
}

void test_cmp_13_tuple_per_position_type_mismatch_context() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("[1,2,true]");

    const auto result = lumalink::json::deserialize<std::tuple<int, std::string, bool>, triple_spec>(fixture.root());
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(failure, 0U, lumalink::json::node_kind::string);
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        1U,
        lumalink::json::node_kind::tuple,
        {},
        {},
        1U);
    lumalink::json::test_support::assert_error_context_entry(failure, 2U, lumalink::json::node_kind::tuple);
}

void test_cmp_14_one_of_ordered_alternative_selection_with_first_successful_match() {
    lumalink::json::test_support::native_fixture bool_fixture;
    bool_fixture.parse_or_fail("true");
    const auto bool_value = lumalink::json::test_support::assert_expected_success(
        lumalink::json::deserialize<std::variant<int, bool>, int_or_bool_spec>(bool_fixture.root()));
    TEST_ASSERT_TRUE(std::holds_alternative<bool>(bool_value));
    TEST_ASSERT_TRUE(std::get<bool>(bool_value));

    lumalink::json::test_support::native_fixture ambiguous_fixture;
    ambiguous_fixture.parse_or_fail("7");
    const auto ambiguous_value = lumalink::json::test_support::assert_expected_success(
        lumalink::json::deserialize<std::variant<int, float>, int_or_number_spec>(ambiguous_fixture.root()));
    TEST_ASSERT_TRUE(std::holds_alternative<int>(ambiguous_value));
    TEST_ASSERT_EQUAL(7, std::get<int>(ambiguous_value));

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<int_or_bool_spec>(std::variant<int, bool>{true}, encoded));
    TEST_ASSERT_TRUE(encoded.as<bool>());
}

void test_cmp_15_one_of_all_alternatives_failed_error_reporting() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"("bogus")");

    const auto result = lumalink::json::deserialize<std::variant<int, bool>, int_or_bool_spec>(fixture.root());
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 2U);
    lumalink::json::test_support::assert_error_context_entry(failure, 0U, lumalink::json::node_kind::integer);
    lumalink::json::test_support::assert_error_context_entry(failure, 1U, lumalink::json::node_kind::one_of);
}

void test_cmp_16_one_of_supported_ambiguity_behavior_uses_first_matching_alternative() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail("5");

    const auto decoded = lumalink::json::deserialize<std::variant<int, float>, int_or_number_spec>(fixture.root());
    const auto& value = lumalink::json::test_support::assert_expected_success(decoded);
    TEST_ASSERT_TRUE(std::holds_alternative<int>(value));
    TEST_ASSERT_EQUAL(5, std::get<int>(value));
}

void run_phase3_composite_tests() {
    RUN_TEST(test_cmp_01_field_and_object_required_field_decode_success);
    RUN_TEST(test_cmp_02_field_and_object_missing_field_failures);
    RUN_TEST(test_cmp_03_field_and_object_nested_field_context_propagation);
    RUN_TEST(test_cmp_04_object_declaration_order_traversal_is_deterministic);
    RUN_TEST(test_cmp_05_optional_present_values);
    RUN_TEST(test_cmp_06_optional_missing_fields);
    RUN_TEST(test_cmp_07_optional_explicit_json_null_values);
    RUN_TEST(test_cmp_08_array_of_homogeneous_container_decode_and_encode);
    RUN_TEST(test_cmp_09_array_of_per_element_type_failures_have_index_context);
    RUN_TEST(test_cmp_10_array_of_min_max_elements_behavior);
    RUN_TEST(test_cmp_11_tuple_exact_positional_decode_and_encode);
    RUN_TEST(test_cmp_12_tuple_array_size_mismatch_failures);
    RUN_TEST(test_cmp_13_tuple_per_position_type_mismatch_context);
    RUN_TEST(test_cmp_14_one_of_ordered_alternative_selection_with_first_successful_match);
    RUN_TEST(test_cmp_15_one_of_all_alternatives_failed_error_reporting);
    RUN_TEST(test_cmp_16_one_of_supported_ambiguity_behavior_uses_first_matching_alternative);
}