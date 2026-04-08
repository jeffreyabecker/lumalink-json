#include <ArduinoJson.h>
#include <unity.h>

#include <array>
#include <expected>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include <json.h>

#include "support/test_support.h"

namespace {

enum class error_test_mode : unsigned char {
    automatic,
    manual,
    orphan,
};

bool validate_even_phase5(const int value) {
    return (value % 2) == 0;
}

bool is_uppercase_phase5(const std::string_view value) {
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

struct sequencing_model {
    int first;
    bool second;
};

struct required_model {
    int required;
};

struct sample_envelope {
    std::vector<std::tuple<int, std::string, bool>> samples;
};

struct named_mode_model {
    error_test_mode mode;
};

using missing_field_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"required", lumalink::json::spec::integer<>>>;
using array_size_mismatch_spec = lumalink::json::spec::tuple<
    lumalink::json::spec::integer<>,
    lumalink::json::spec::string<>,
    lumalink::json::spec::boolean<>>;
using validation_failure_spec =
    lumalink::json::spec::integer<lumalink::json::opts::validator_func<validate_even_phase5>>;
using bounded_integer_spec_phase5 = lumalink::json::spec::integer<lumalink::json::opts::min_max_value<10, 20>>;
using uppercase_string_spec_phase5 =
    lumalink::json::spec::string<lumalink::json::opts::pattern<is_uppercase_phase5>>;
using sequencing_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"first", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"second", lumalink::json::spec::boolean<>>>;
using sample_tuple_spec = lumalink::json::spec::tuple<
    lumalink::json::spec::integer<lumalink::json::opts::name<"sample_value">>,
    lumalink::json::spec::string<>,
    lumalink::json::spec::boolean<>>;
using sample_envelope_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"samples", lumalink::json::spec::array_of<sample_tuple_spec>>>;
using named_mode_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<
        "mode",
        lumalink::json::spec::enum_string<error_test_mode, lumalink::json::opts::name<"selected_mode">>>>;

} // namespace

namespace lumalink::json::traits {

template <>
struct enum_strings<error_test_mode> {
    static constexpr std::array<enum_mapping_entry<error_test_mode>, 2> values{{
        {"auto", error_test_mode::automatic},
        {"manual", error_test_mode::manual},
    }};
};

} // namespace lumalink::json::traits

void test_err_01_every_supported_runtime_error_code_is_covered() {
    lumalink::json::test_support::native_fixture missing_fixture;
    missing_fixture.parse_or_fail("{}");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<required_model, missing_field_spec>(missing_fixture.root()),
        lumalink::json::error_code::missing_field);

    lumalink::json::test_support::native_fixture type_fixture;
    type_fixture.parse_or_fail(R"("bad")");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, lumalink::json::spec::integer<>>(type_fixture.root()),
        lumalink::json::error_code::unexpected_type);

    lumalink::json::test_support::native_fixture tuple_fixture;
    tuple_fixture.parse_or_fail("[1,true]");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<std::tuple<int, std::string, bool>, array_size_mismatch_spec>(tuple_fixture.root()),
        lumalink::json::error_code::array_size_mismatch);

    lumalink::json::test_support::native_fixture validator_fixture;
    validator_fixture.parse_or_fail("11");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, validation_failure_spec>(validator_fixture.root()),
        lumalink::json::error_code::validation_failed);

    lumalink::json::test_support::native_fixture range_fixture;
    range_fixture.parse_or_fail("25");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<int, bounded_integer_spec_phase5>(range_fixture.root()),
        lumalink::json::error_code::value_out_of_range);

    lumalink::json::test_support::native_fixture pattern_fixture;
    pattern_fixture.parse_or_fail(R"("Auto")");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<std::string, uppercase_string_spec_phase5>(pattern_fixture.root()),
        lumalink::json::error_code::pattern_mismatch);

    lumalink::json::test_support::native_fixture unknown_enum_fixture;
    unknown_enum_fixture.parse_or_fail(R"("bogus")");
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::deserialize<error_test_mode, lumalink::json::spec::enum_string<error_test_mode>>(
            unknown_enum_fixture.root()),
        lumalink::json::error_code::enum_string_unknown);

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_error(
        lumalink::json::serialize<lumalink::json::spec::enum_string<error_test_mode>>(error_test_mode::orphan, encoded),
        lumalink::json::error_code::enum_value_unmapped);

    lumalink::json::test_support::assert_expected_error(
        lumalink::json::not_implemented(),
        lumalink::json::error_code::not_implemented);
}

void test_err_02_full_context_policy_retains_full_path() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"samples":[["bad","READY",true]]})");

    const auto result = lumalink::json::deserialize<sample_envelope, sample_envelope_spec>(
        fixture.root(),
        lumalink::json::decode_options{lumalink::json::context_policy::full});
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 7U);
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        0U,
        lumalink::json::node_kind::integer,
        {},
        "sample_value");
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        1U,
        lumalink::json::node_kind::tuple,
        {},
        {},
        0U);
    lumalink::json::test_support::assert_error_context_entry(failure, 2U, lumalink::json::node_kind::tuple);
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        3U,
        lumalink::json::node_kind::array_of,
        {},
        {},
        0U);
    lumalink::json::test_support::assert_error_context_entry(failure, 4U, lumalink::json::node_kind::array_of);
    lumalink::json::test_support::assert_error_context_entry(failure, 5U, lumalink::json::node_kind::field, "samples");
    lumalink::json::test_support::assert_error_context_entry(failure, 6U, lumalink::json::node_kind::object);
}

void test_err_03_last_context_policy_retains_outermost_entry() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"samples":[["bad","READY",true]]})");

    const auto result = lumalink::json::deserialize<sample_envelope, sample_envelope_spec>(
        fixture.root(),
        lumalink::json::decode_options{lumalink::json::context_policy::last});
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 1U);
    lumalink::json::test_support::assert_error_context_entry(failure, 0U, lumalink::json::node_kind::object);
}

void test_err_04_none_context_policy_omits_context() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"samples":[["bad","READY",true]]})");

    const auto result = lumalink::json::deserialize<sample_envelope, sample_envelope_spec>(
        fixture.root(),
        lumalink::json::decode_options{lumalink::json::context_policy::none});
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 0U);
}

void test_err_05_first_failure_is_returned_without_hidden_recovery() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"first":"bad","second":"also-bad"})");

    const auto result = lumalink::json::deserialize<sequencing_model, sequencing_spec>(fixture.root());
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(failure, 0U, lumalink::json::node_kind::integer);
    lumalink::json::test_support::assert_error_context_entry(failure, 1U, lumalink::json::node_kind::field, "first");
    lumalink::json::test_support::assert_error_context_entry(failure, 2U, lumalink::json::node_kind::object);
}

void test_err_06_nested_object_array_tuple_scalar_context_is_index_aware() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"samples":[[9,"READY",true],["bad","READY",true]]})");

    const auto result = lumalink::json::deserialize<sample_envelope, sample_envelope_spec>(fixture.root());
    const auto& failure =
        lumalink::json::test_support::assert_expected_error(result, lumalink::json::error_code::unexpected_type);
    lumalink::json::test_support::assert_error_context_depth(failure, 7U);
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        1U,
        lumalink::json::node_kind::tuple,
        {},
        {},
        0U);
    lumalink::json::test_support::assert_error_context_entry(failure, 2U, lumalink::json::node_kind::tuple);
    lumalink::json::test_support::assert_error_context_entry(
        failure,
        3U,
        lumalink::json::node_kind::array_of,
        {},
        {},
        1U);
    lumalink::json::test_support::assert_error_context_entry(failure, 4U, lumalink::json::node_kind::array_of);
}

void test_err_07_enum_mapping_failures_include_node_kind_field_key_and_logical_name() {
    lumalink::json::test_support::native_fixture decode_fixture;
    decode_fixture.parse_or_fail(R"({"mode":"bogus"})");

    const auto decoded = lumalink::json::deserialize<named_mode_model, named_mode_spec>(decode_fixture.root());
    const auto& decode_failure =
        lumalink::json::test_support::assert_expected_error(decoded, lumalink::json::error_code::enum_string_unknown);
    lumalink::json::test_support::assert_error_context_depth(decode_failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(
        decode_failure,
        0U,
        lumalink::json::node_kind::enum_string,
        {},
        "selected_mode");
    lumalink::json::test_support::assert_error_context_entry(
        decode_failure,
        1U,
        lumalink::json::node_kind::field,
        "mode");
    lumalink::json::test_support::assert_error_context_entry(decode_failure, 2U, lumalink::json::node_kind::object);

    JsonDocument encoded;
    const auto encoded_result = lumalink::json::serialize<named_mode_spec>(named_mode_model{error_test_mode::orphan}, encoded);
    const auto& encode_failure =
        lumalink::json::test_support::assert_expected_error(encoded_result, lumalink::json::error_code::enum_value_unmapped);
    lumalink::json::test_support::assert_error_context_depth(encode_failure, 3U);
    lumalink::json::test_support::assert_error_context_entry(
        encode_failure,
        0U,
        lumalink::json::node_kind::enum_string,
        {},
        "selected_mode");
    lumalink::json::test_support::assert_error_context_entry(
        encode_failure,
        1U,
        lumalink::json::node_kind::field,
        "mode");
    lumalink::json::test_support::assert_error_context_entry(encode_failure, 2U, lumalink::json::node_kind::object);
}

void test_err_08_validation_error_codes_are_differentiated() {
    lumalink::json::test_support::native_fixture range_fixture;
    range_fixture.parse_or_fail("25");
    const auto range_result = lumalink::json::deserialize<int, bounded_integer_spec_phase5>(range_fixture.root());
    const auto& range_failure =
        lumalink::json::test_support::assert_expected_error(range_result, lumalink::json::error_code::value_out_of_range);

    lumalink::json::test_support::native_fixture pattern_fixture;
    pattern_fixture.parse_or_fail(R"("Auto")");
    const auto pattern_result =
        lumalink::json::deserialize<std::string, uppercase_string_spec_phase5>(pattern_fixture.root());
    const auto& pattern_failure = lumalink::json::test_support::assert_expected_error(
        pattern_result,
        lumalink::json::error_code::pattern_mismatch);

    lumalink::json::test_support::native_fixture validator_fixture;
    validator_fixture.parse_or_fail("11");
    const auto validation_result = lumalink::json::deserialize<int, validation_failure_spec>(validator_fixture.root());
    const auto& validation_failure = lumalink::json::test_support::assert_expected_error(
        validation_result,
        lumalink::json::error_code::validation_failed);

    TEST_ASSERT_NOT_EQUAL(
        static_cast<int>(range_failure.code),
        static_cast<int>(pattern_failure.code));
    TEST_ASSERT_NOT_EQUAL(
        static_cast<int>(pattern_failure.code),
        static_cast<int>(validation_failure.code));
    TEST_ASSERT_NOT_EQUAL(
        static_cast<int>(range_failure.code),
        static_cast<int>(validation_failure.code));
}

void run_phase5_error_tests() {
    RUN_TEST(test_err_01_every_supported_runtime_error_code_is_covered);
    RUN_TEST(test_err_02_full_context_policy_retains_full_path);
    RUN_TEST(test_err_03_last_context_policy_retains_outermost_entry);
    RUN_TEST(test_err_04_none_context_policy_omits_context);
    RUN_TEST(test_err_05_first_failure_is_returned_without_hidden_recovery);
    RUN_TEST(test_err_06_nested_object_array_tuple_scalar_context_is_index_aware);
    RUN_TEST(test_err_07_enum_mapping_failures_include_node_kind_field_key_and_logical_name);
    RUN_TEST(test_err_08_validation_error_codes_are_differentiated);
}