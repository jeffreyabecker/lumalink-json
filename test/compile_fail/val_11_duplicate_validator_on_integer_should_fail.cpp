#include <json.h>

constexpr bool validator_one(const int) {
    return true;
}

constexpr bool validator_two(const int) {
    return true;
}

using duplicate_validator_spec = lumalink::json::spec::integer<
    lumalink::json::opts::validator_func<validator_one>,
    lumalink::json::opts::validator_func<validator_two>>;

duplicate_validator_spec g_duplicate_validator_spec{};