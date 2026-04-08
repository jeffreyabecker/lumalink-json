#include <ArduinoJson.h>
#include <unity.h>

#include <optional>
#include <string>
#include <tuple>

#include <LumaLinkJson.h>

#include "support/test_support.h"

namespace {

class manual_device_settings {
public:
    manual_device_settings() : id(0), label(), enabled(false) {}

    int id;
    std::string label;
    bool enabled;
};

class manual_optional_counter {
public:
    manual_optional_counter() : count() {}

    std::optional<int> count;
};

using manual_settings_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"display_name", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"enabled", lumalink::json::spec::boolean<>>>;

using manual_optional_counter_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"count", lumalink::json::spec::optional<lumalink::json::spec::integer<>>>>;

} // namespace

namespace lumalink::json::traits {

template <>
struct object_fields<manual_device_settings> {
    static constexpr auto members = std::make_tuple(
        &manual_device_settings::id,
        &manual_device_settings::label,
        &manual_device_settings::enabled);
};

template <>
struct object_fields<manual_optional_counter> {
    static constexpr auto members = std::make_tuple(&manual_optional_counter::count);
};

} // namespace lumalink::json::traits

namespace {

void test_rfl_03_explicit_field_traits_bind_non_aggregate_models() {
    lumalink::json::test_support::native_fixture fixture;
    fixture.parse_or_fail(R"({"id":11,"display_name":"MANUAL","enabled":true})");

    const auto decoded = lumalink::json::deserialize<manual_device_settings, manual_settings_spec>(fixture.root());
    const auto& value = lumalink::json::test_support::assert_expected_success(decoded);
    TEST_ASSERT_EQUAL(11, value.id);
    TEST_ASSERT_EQUAL_STRING("MANUAL", value.label.c_str());
    TEST_ASSERT_TRUE(value.enabled);

    manual_device_settings encoded_value;
    encoded_value.id = 7;
    encoded_value.label = "AUTO";
    encoded_value.enabled = false;

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<manual_settings_spec>(encoded_value, encoded));
    std::string serialized;
    serializeJson(encoded, serialized);
    TEST_ASSERT_EQUAL_STRING("{\"id\":7,\"display_name\":\"AUTO\",\"enabled\":false}", serialized.c_str());
}

void test_rfl_03_explicit_field_traits_handle_optional_members() {
    lumalink::json::test_support::native_fixture missing_fixture;
    missing_fixture.parse_or_fail(R"({})");

    const auto missing = lumalink::json::deserialize<manual_optional_counter, manual_optional_counter_spec>(
        missing_fixture.root());
    const auto& missing_value = lumalink::json::test_support::assert_expected_success(missing);
    TEST_ASSERT_FALSE(missing_value.count.has_value());

    lumalink::json::test_support::native_fixture present_fixture;
    present_fixture.parse_or_fail(R"({"count":12})");

    const auto present = lumalink::json::deserialize<manual_optional_counter, manual_optional_counter_spec>(
        present_fixture.root());
    const auto& present_value = lumalink::json::test_support::assert_expected_success(present);
    TEST_ASSERT_TRUE(present_value.count.has_value());
    TEST_ASSERT_EQUAL(12, *present_value.count);
}

} // namespace

void run_phase8_reflection_tests() {
    RUN_TEST(test_rfl_03_explicit_field_traits_bind_non_aggregate_models);
    RUN_TEST(test_rfl_03_explicit_field_traits_handle_optional_members);
}