#include <string>
#include <vector>

#include <LumaLinkJson.h>

std::string invalid_array_validator(const std::vector<int>&) {
    return "bad";
}

using invalid_array_validator_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::validator_func<invalid_array_validator>>;

auto trigger_invalid_array_validator_failure() {
    JsonDocument document;
    return lumalink::json::deserialize<std::vector<int>, invalid_array_validator_spec>(document.as<JsonVariantConst>());
}