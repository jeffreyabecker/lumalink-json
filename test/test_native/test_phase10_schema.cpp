#include <ArduinoJson.h>
#include <unity.h>

#include <array>
#include <string>
#include <string_view>

#include <LumaLinkJson.h>

#include "support/test_support.h"

namespace {

enum class schema_mode : unsigned char {
    automatic,
    manual,
    disabled,
};

constexpr bool is_uppercase_schema_value(const std::string_view value) {
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

using basic_object_schema_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"label", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"count", lumalink::json::spec::optional<lumalink::json::spec::integer<>>>>;

using plain_array_schema_spec = lumalink::json::spec::array_of<lumalink::json::spec::integer<>>;
using bounded_array_schema_spec =
    lumalink::json::spec::array_of<lumalink::json::spec::integer<>, lumalink::json::opts::min_max_elements<1, 3>>;
using tuple_schema_spec = lumalink::json::spec::tuple<
    lumalink::json::spec::integer<>,
    lumalink::json::spec::string<>,
    lumalink::json::spec::boolean<>>;
using one_of_schema_spec =
    lumalink::json::spec::one_of<lumalink::json::spec::integer<>, lumalink::json::spec::string<>>;
using enum_schema_spec = lumalink::json::spec::enum_string<schema_mode>;
using bounded_integer_schema_spec = lumalink::json::spec::integer<lumalink::json::opts::min_max_value<10, 20>>;
using patterned_string_schema_spec =
    lumalink::json::spec::string<lumalink::json::opts::pattern<is_uppercase_schema_value, "^[A-Z]+$">>;

std::string serialize_schema_or_fail(const JsonDocument& document) {
    std::string serialized;
    const size_t bytes_written = serializeJson(document, serialized);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, static_cast<uint32_t>(bytes_written));
    return serialized;
}

} // namespace

namespace lumalink::json::traits {

template <>
struct enum_strings<schema_mode> {
    static constexpr std::array<enum_mapping_entry<schema_mode>, 3> values{{
        {"auto", schema_mode::automatic},
        {"manual", schema_mode::manual},
        {"disabled", schema_mode::disabled},
    }};
};

} // namespace lumalink::json::traits

void test_sch_01_scalar_nodes_emit_type_declarations() {
    JsonDocument null_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<lumalink::json::spec::null<>>(null_schema));
    TEST_ASSERT_EQUAL_STRING("{\"type\":\"null\"}", serialize_schema_or_fail(null_schema).c_str());

    JsonDocument boolean_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<lumalink::json::spec::boolean<>>(boolean_schema));
    TEST_ASSERT_EQUAL_STRING("{\"type\":\"boolean\"}", serialize_schema_or_fail(boolean_schema).c_str());

    JsonDocument integer_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<lumalink::json::spec::integer<>>(integer_schema));
    TEST_ASSERT_EQUAL_STRING("{\"type\":\"integer\"}", serialize_schema_or_fail(integer_schema).c_str());

    JsonDocument number_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<lumalink::json::spec::number<>>(number_schema));
    TEST_ASSERT_EQUAL_STRING("{\"type\":\"number\"}", serialize_schema_or_fail(number_schema).c_str());

    JsonDocument string_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<lumalink::json::spec::string<>>(string_schema));
    TEST_ASSERT_EQUAL_STRING("{\"type\":\"string\"}", serialize_schema_or_fail(string_schema).c_str());

    JsonDocument any_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<lumalink::json::spec::any<>>(any_schema));
    TEST_ASSERT_EQUAL_STRING("{}", serialize_schema_or_fail(any_schema).c_str());
}

void test_sch_02_field_and_object_emit_properties_required_and_stable_order() {
    JsonDocument schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<basic_object_schema_spec>(schema));

    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"object\",\"properties\":{\"id\":{\"type\":\"integer\"},\"label\":{\"type\":\"string\"},\"count\":{\"anyOf\":[{\"type\":\"integer\"},{\"type\":\"null\"}]}},\"required\":[\"id\",\"label\"]}",
        serialize_schema_or_fail(schema).c_str());
}

void test_sch_03_optional_array_tuple_and_one_of_emit_supported_forms() {
    JsonDocument optional_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<lumalink::json::spec::optional<lumalink::json::spec::integer<>>>(optional_schema));
    TEST_ASSERT_EQUAL_STRING(
        "{\"anyOf\":[{\"type\":\"integer\"},{\"type\":\"null\"}]}",
        serialize_schema_or_fail(optional_schema).c_str());

    JsonDocument array_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<plain_array_schema_spec>(array_schema));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"array\",\"items\":{\"type\":\"integer\"}}",
        serialize_schema_or_fail(array_schema).c_str());

    JsonDocument tuple_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<tuple_schema_spec>(tuple_schema));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"array\",\"minItems\":3,\"maxItems\":3,\"prefixItems\":[{\"type\":\"integer\"},{\"type\":\"string\"},{\"type\":\"boolean\"}]}",
        serialize_schema_or_fail(tuple_schema).c_str());

    JsonDocument one_of_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<one_of_schema_spec>(one_of_schema));
    TEST_ASSERT_EQUAL_STRING(
        "{\"anyOf\":[{\"type\":\"integer\"},{\"type\":\"string\"}]}",
        serialize_schema_or_fail(one_of_schema).c_str());
}

void test_sch_04_enum_string_mappings_emit_enum_token_arrays() {
    JsonDocument schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<enum_schema_spec>(schema));

    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"string\",\"enum\":[\"auto\",\"manual\",\"disabled\"]}",
        serialize_schema_or_fail(schema).c_str());
}

void test_sch_05_supported_options_project_schema_keywords() {
    JsonDocument integer_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<bounded_integer_schema_spec>(integer_schema));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"integer\",\"minimum\":10,\"maximum\":20}",
        serialize_schema_or_fail(integer_schema).c_str());

    JsonDocument array_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<bounded_array_schema_spec>(array_schema));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"array\",\"minItems\":1,\"maxItems\":3,\"items\":{\"type\":\"integer\"}}",
        serialize_schema_or_fail(array_schema).c_str());

    JsonDocument string_schema;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<patterned_string_schema_spec>(string_schema));
    TEST_ASSERT_EQUAL_STRING(
        "{\"type\":\"string\",\"pattern\":\"^[A-Z]+$\"}",
        serialize_schema_or_fail(string_schema).c_str());
}

void test_sch_06_schema_generation_writes_to_caller_provided_document() {
    JsonDocument schema;
    schema["stale"] = true;

    lumalink::json::test_support::assert_expected_success(
        lumalink::json::generate_schema<lumalink::json::spec::boolean<>>(schema));
    TEST_ASSERT_EQUAL_STRING("{\"type\":\"boolean\"}", serialize_schema_or_fail(schema).c_str());
}

void run_phase10_schema_tests() {
    RUN_TEST(test_sch_01_scalar_nodes_emit_type_declarations);
    RUN_TEST(test_sch_02_field_and_object_emit_properties_required_and_stable_order);
    RUN_TEST(test_sch_03_optional_array_tuple_and_one_of_emit_supported_forms);
    RUN_TEST(test_sch_04_enum_string_mappings_emit_enum_token_arrays);
    RUN_TEST(test_sch_05_supported_options_project_schema_keywords);
    RUN_TEST(test_sch_06_schema_generation_writes_to_caller_provided_document);
}