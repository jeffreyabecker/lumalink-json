#include <json.h>

namespace {

union union_model {
    int value;
    bool enabled;
};

using model_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"value", lumalink::json::spec::integer<>>>;

} // namespace

auto trigger_union_failure() {
    JsonDocument document;
    return lumalink::json::deserialize<union_model, model_spec>(document.as<JsonVariantConst>());
}