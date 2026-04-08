#include <json.h>

using duplicate_name_spec = lumalink::json::spec::integer<
    lumalink::json::opts::name<"first">,
    lumalink::json::opts::name<"second">>;

duplicate_name_spec g_duplicate_name_spec{};