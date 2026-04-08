#include <string_view>

#include <json.h>

constexpr bool allow_any_string(std::string_view) {
    return true;
}

constexpr bool allow_uppercase_string(std::string_view) {
    return true;
}

using duplicate_pattern_spec = lumalink::json::spec::string<
    lumalink::json::opts::pattern<allow_any_string>,
    lumalink::json::opts::pattern<allow_uppercase_string>>;

duplicate_pattern_spec g_duplicate_pattern_spec{};