#include <LumaLinkJson.h>

int invalid_integer_validator(const int) {
    return 1;
}

using invalid_integer_validator_spec =
    lumalink::json::spec::integer<lumalink::json::opts::validator_func<invalid_integer_validator>>;

auto trigger_invalid_integer_validator_failure() {
    JsonDocument document;
    return lumalink::json::deserialize<int, invalid_integer_validator_spec>(document.as<JsonVariantConst>());
}