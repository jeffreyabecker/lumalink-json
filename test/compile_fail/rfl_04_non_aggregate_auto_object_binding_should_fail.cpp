#include <LumaLinkJson.h>

namespace {

struct non_aggregate_model {
    int value;

    non_aggregate_model() : value(0) {}
};

using model_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"value", lumalink::json::spec::integer<>>>;

} // namespace

auto trigger_non_aggregate_failure() {
    JsonDocument document;
    return lumalink::json::deserialize<non_aggregate_model, model_spec>(document.as<JsonVariantConst>());
}