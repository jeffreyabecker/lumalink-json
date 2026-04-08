#include <json.h>

void invalid_void_validator(const int) {}

using invalid_void_validator_spec =
    lumalink::json::spec::integer<lumalink::json::opts::validator_func<invalid_void_validator>>;

auto trigger_invalid_void_validator_failure() {
    JsonDocument document;
    return lumalink::json::deserialize<int, invalid_void_validator_spec>(document.as<JsonVariantConst>());
}