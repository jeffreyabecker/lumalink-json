#include <LumaLinkJson.h>

using duplicate_not_empty_spec = lumalink::json::spec::string<
    lumalink::json::opts::not_empty<"first message">,
    lumalink::json::opts::not_empty<"second message">>;

duplicate_not_empty_spec g_duplicate_not_empty_spec{};