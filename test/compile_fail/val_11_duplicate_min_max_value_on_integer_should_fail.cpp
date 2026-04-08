#include <json.h>

using duplicate_range_spec = lumalink::json::spec::integer<
    lumalink::json::opts::min_max_value<0, 10>,
    lumalink::json::opts::min_max_value<5, 15>>;

duplicate_range_spec g_duplicate_range_spec{};