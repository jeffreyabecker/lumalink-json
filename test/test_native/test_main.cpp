#include <ArduinoJson.h>
#include <unity.h>

#include <json.h>

void setUp(void) {}

void tearDown(void) {}

void test_arduinojson_roundtrip_string_value() {
    JsonDocument doc;
    doc["mode"] = "auto";

    const char* mode = doc["mode"].as<const char*>();
    TEST_ASSERT_NOT_NULL(mode);
    TEST_ASSERT_EQUAL_STRING("auto", mode);
}

void test_expected_error_alias_smoke() {
    const auto result = lumalink::json::not_implemented();

    TEST_ASSERT_FALSE(result.has_value());
    TEST_ASSERT_EQUAL(
        static_cast<int>(lumalink::json::error_code::not_implemented),
        static_cast<int>(result.error().code));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_arduinojson_roundtrip_string_value);
    RUN_TEST(test_expected_error_alias_smoke);
    return UNITY_END();
}
