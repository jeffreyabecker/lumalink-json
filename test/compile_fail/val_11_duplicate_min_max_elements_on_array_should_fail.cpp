#include <LumaLinkJson.h>

using duplicate_element_bounds_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::min_max_elements<1, 3>,
    lumalink::json::opts::min_max_elements<2, 4>>;

duplicate_element_bounds_spec g_duplicate_element_bounds_spec{};