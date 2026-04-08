#include <LumaLinkJson.h>

namespace {

struct base_model {
    int inherited_value;
};

struct derived_model : base_model {
    int leaf_value;
};

using model_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"inherited_value", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"leaf_value", lumalink::json::spec::integer<>>>;

} // namespace

auto trigger_inherited_failure() {
    JsonDocument document;
    return lumalink::json::deserialize<derived_model, model_spec>(document.as<JsonVariantConst>());
}