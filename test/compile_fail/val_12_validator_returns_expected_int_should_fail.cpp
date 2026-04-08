#include <expected>

#include <json.h>

std::expected<int, lumalink::json::error_code> invalid_expected_validator(const int) {
    return 1;
}

using invalid_expected_validator_spec =
    lumalink::json::spec::integer<lumalink::json::opts::validator_func<invalid_expected_validator>>;

auto trigger_invalid_expected_validator_failure() {
    JsonDocument document;
    return lumalink::json::deserialize<int, invalid_expected_validator_spec>(document.as<JsonVariantConst>());
}