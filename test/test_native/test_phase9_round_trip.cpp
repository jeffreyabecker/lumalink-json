#include <ArduinoJson.h>
#include <unity.h>

#include <array>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

#include <json.h>

#include "support/test_support.h"

namespace {

enum class round_trip_mode : unsigned char {
    automatic,
    manual,
    disabled,
};

constexpr bool is_even_round_trip(const int value) {
    return (value % 2) == 0;
}

struct round_trip_profile {
    int level;
    bool active;
};

struct round_trip_envelope {
    round_trip_profile profile;
    std::string label;
};

struct round_trip_collection {
    std::vector<int> samples;
    std::optional<int> count;
    std::tuple<int, std::string, bool> triple;
};

struct round_trip_validated_leaf {
    int threshold;
    round_trip_mode mode;
};

struct round_trip_validated_envelope {
    std::string label;
    round_trip_validated_leaf config;
    std::vector<int> steps;
};

using round_trip_profile_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"level", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"active", lumalink::json::spec::boolean<>>>;

using round_trip_envelope_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"profile", round_trip_profile_spec>,
    lumalink::json::spec::field<"label", lumalink::json::spec::string<>>>;

using round_trip_collection_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"samples", lumalink::json::spec::array_of<lumalink::json::spec::integer<>>>,
    lumalink::json::spec::field<"count", lumalink::json::spec::optional<lumalink::json::spec::integer<>>>,
    lumalink::json::spec::field<
        "triple",
        lumalink::json::spec::tuple<
            lumalink::json::spec::integer<>,
            lumalink::json::spec::string<>,
            lumalink::json::spec::boolean<>>>>;

using round_trip_mode_spec = lumalink::json::spec::enum_string<round_trip_mode>;
using int_or_bool_spec =
    lumalink::json::spec::one_of<lumalink::json::spec::integer<>, lumalink::json::spec::boolean<>>;
using int_or_number_spec =
    lumalink::json::spec::one_of<lumalink::json::spec::integer<>, lumalink::json::spec::number<>>;

using round_trip_validated_leaf_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<
        "threshold",
        lumalink::json::spec::integer<
            lumalink::json::opts::name<"even_threshold">,
            lumalink::json::opts::validator_func<is_even_round_trip>>>,
    lumalink::json::spec::field<
        "mode",
        lumalink::json::spec::enum_string<round_trip_mode, lumalink::json::opts::name<"selected_mode">>>>;

using round_trip_validated_envelope_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"label", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"config", round_trip_validated_leaf_spec>,
    lumalink::json::spec::field<
        "steps",
        lumalink::json::spec::array_of<
            lumalink::json::spec::integer<>,
            lumalink::json::opts::min_max_elements<1, 4>>>>;

template <typename Model, typename Spec>
lumalink::json::expected<Model> round_trip_model(const Model& model) {
    return lumalink::json::test_support::round_trip<Model>(
        model,
        [](const Model& value, JsonDocument& document) -> lumalink::json::expected_void {
            return lumalink::json::serialize<Spec>(value, document);
        },
        [](const JsonVariantConst root) -> lumalink::json::expected<Model> {
            return lumalink::json::deserialize<Model, Spec>(root);
        });
}

std::string serialize_document_or_fail(const JsonDocument& document) {
    std::string serialized;
    const size_t written = serializeJson(document, serialized);
    TEST_ASSERT_GREATER_THAN_UINT32(0U, static_cast<uint32_t>(written));
    return serialized;
}

} // namespace

namespace lumalink::json::traits {

template <>
struct enum_strings<round_trip_mode> {
    static constexpr std::array<enum_mapping_entry<round_trip_mode>, 3> values{{
        {"auto", round_trip_mode::automatic},
        {"manual", round_trip_mode::manual},
        {"disabled", round_trip_mode::disabled},
    }};
};

} // namespace lumalink::json::traits

void test_rt_01_scalar_nodes_round_trip() {
    const auto null_result = lumalink::json::test_support::round_trip<std::nullptr_t>(
        nullptr,
        [](std::nullptr_t value, JsonDocument& document) -> lumalink::json::expected_void {
            return lumalink::json::serialize<lumalink::json::spec::null<>>(value, document);
        },
        [](const JsonVariantConst root) -> lumalink::json::expected<std::nullptr_t> {
            return lumalink::json::deserialize<std::nullptr_t, lumalink::json::spec::null<>>(root);
        });
    TEST_ASSERT_NULL(lumalink::json::test_support::assert_expected_success(null_result));

    const auto bool_result = round_trip_model<bool, lumalink::json::spec::boolean<>>(true);
    TEST_ASSERT_TRUE(lumalink::json::test_support::assert_expected_success(bool_result));

    const auto integer_result = round_trip_model<int, lumalink::json::spec::integer<>>(17);
    TEST_ASSERT_EQUAL(17, lumalink::json::test_support::assert_expected_success(integer_result));

    const auto number_result = round_trip_model<double, lumalink::json::spec::number<>>(2.5);
    TEST_ASSERT_TRUE(std::fabs(lumalink::json::test_support::assert_expected_success(number_result) - 2.5) < 0.0001);

    const auto string_result = round_trip_model<std::string, lumalink::json::spec::string<>>(std::string{"AUTO"});
    TEST_ASSERT_EQUAL_STRING(
        "AUTO",
        lumalink::json::test_support::assert_expected_success(string_result).c_str());
}

void test_rt_02_nested_objects_round_trip() {
    const round_trip_envelope input{{9, true}, "PRIMARY"};

    const auto result = round_trip_model<round_trip_envelope, round_trip_envelope_spec>(input);
    const auto& value = lumalink::json::test_support::assert_expected_success(result);
    TEST_ASSERT_EQUAL(9, value.profile.level);
    TEST_ASSERT_TRUE(value.profile.active);
    TEST_ASSERT_EQUAL_STRING("PRIMARY", value.label.c_str());
}

void test_rt_03_arrays_tuples_and_optionals_round_trip() {
    const round_trip_collection with_count{{1, 2, 3}, 5, std::make_tuple(8, std::string{"AUTO"}, true)};
    const auto with_count_result = round_trip_model<round_trip_collection, round_trip_collection_spec>(with_count);
    const auto& with_count_value = lumalink::json::test_support::assert_expected_success(with_count_result);
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(with_count_value.samples.size()));
    TEST_ASSERT_TRUE(with_count_value.count.has_value());
    TEST_ASSERT_EQUAL(5, *with_count_value.count);
    TEST_ASSERT_EQUAL(8, std::get<0>(with_count_value.triple));
    TEST_ASSERT_EQUAL_STRING("AUTO", std::get<1>(with_count_value.triple).c_str());
    TEST_ASSERT_TRUE(std::get<2>(with_count_value.triple));

    const round_trip_collection without_count{{4, 5}, std::nullopt, std::make_tuple(1, std::string{"MANUAL"}, false)};
    const auto without_count_result = round_trip_model<round_trip_collection, round_trip_collection_spec>(without_count);
    const auto& without_count_value = lumalink::json::test_support::assert_expected_success(without_count_result);
    TEST_ASSERT_FALSE(without_count_value.count.has_value());
    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(without_count_value.samples.size()));
    TEST_ASSERT_FALSE(std::get<2>(without_count_value.triple));
}

void test_rt_04_enums_round_trip_through_canonical_tokens() {
    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<round_trip_mode_spec>(round_trip_mode::manual, encoded));
    TEST_ASSERT_EQUAL_STRING("\"manual\"", serialize_document_or_fail(encoded).c_str());

    const auto result = round_trip_model<round_trip_mode, round_trip_mode_spec>(round_trip_mode::manual);
    TEST_ASSERT_EQUAL(
        static_cast<int>(round_trip_mode::manual),
        static_cast<int>(lumalink::json::test_support::assert_expected_success(result)));
}

void test_rt_05_one_of_round_trip_each_supported_alternative() {
    const auto int_result = round_trip_model<std::variant<int, bool>, int_or_bool_spec>(std::variant<int, bool>{7});
    const auto& int_value = lumalink::json::test_support::assert_expected_success(int_result);
    TEST_ASSERT_TRUE(std::holds_alternative<int>(int_value));
    TEST_ASSERT_EQUAL(7, std::get<int>(int_value));

    const auto bool_result = round_trip_model<std::variant<int, bool>, int_or_bool_spec>(std::variant<int, bool>{true});
    const auto& bool_value = lumalink::json::test_support::assert_expected_success(bool_result);
    TEST_ASSERT_TRUE(std::holds_alternative<bool>(bool_value));
    TEST_ASSERT_TRUE(std::get<bool>(bool_value));

    const auto float_result = round_trip_model<std::variant<int, float>, int_or_number_spec>(std::variant<int, float>{2.5f});
    const auto& float_value = lumalink::json::test_support::assert_expected_success(float_result);
    TEST_ASSERT_TRUE(std::holds_alternative<float>(float_value));
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 2.5f, std::get<float>(float_value));
}

void test_rt_06_nested_validators_and_enum_mapping_round_trip() {
    const round_trip_validated_envelope input{
        "ACTIVE",
        {12, round_trip_mode::disabled},
        {2, 4, 6}};

    JsonDocument encoded;
    lumalink::json::test_support::assert_expected_success(
        lumalink::json::serialize<round_trip_validated_envelope_spec>(input, encoded));
    TEST_ASSERT_EQUAL_STRING(
        "{\"label\":\"ACTIVE\",\"config\":{\"threshold\":12,\"mode\":\"disabled\"},\"steps\":[2,4,6]}",
        serialize_document_or_fail(encoded).c_str());

    const auto result = round_trip_model<round_trip_validated_envelope, round_trip_validated_envelope_spec>(input);
    const auto& value = lumalink::json::test_support::assert_expected_success(result);
    TEST_ASSERT_EQUAL_STRING("ACTIVE", value.label.c_str());
    TEST_ASSERT_EQUAL(12, value.config.threshold);
    TEST_ASSERT_EQUAL(static_cast<int>(round_trip_mode::disabled), static_cast<int>(value.config.mode));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(value.steps.size()));
    TEST_ASSERT_EQUAL(6, value.steps[2]);
}

void run_phase9_round_trip_tests() {
    RUN_TEST(test_rt_01_scalar_nodes_round_trip);
    RUN_TEST(test_rt_02_nested_objects_round_trip);
    RUN_TEST(test_rt_03_arrays_tuples_and_optionals_round_trip);
    RUN_TEST(test_rt_04_enums_round_trip_through_canonical_tokens);
    RUN_TEST(test_rt_05_one_of_round_trip_each_supported_alternative);
    RUN_TEST(test_rt_06_nested_validators_and_enum_mapping_round_trip);
}

void test_rt_07_jsondocument_member_round_trip() {
    struct doc_holder {
        JsonDocument doc;
    } input;

    // populate the embedded document
    JsonVariant root = input.doc.to<JsonVariant>();
    root["x"] = 42;

    using doc_holder_spec = lumalink::json::spec::object<
        lumalink::json::spec::field<"doc", lumalink::json::spec::any<>>>;

    const auto result = round_trip_model<doc_holder, doc_holder_spec>(input);
    const auto& value = lumalink::json::test_support::assert_expected_success(result);

    const JsonVariantConst out_root = value.doc.as<JsonVariantConst>();
    TEST_ASSERT_EQUAL(42, out_root["x"].as<int>());
}