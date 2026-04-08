#include <LumaLinkJson.h>

namespace {

struct polymorphic_model {
    int value;
    virtual ~polymorphic_model() = default;
};

using model_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"value", lumalink::json::spec::integer<>>>;

} // namespace

auto trigger_polymorphic_failure() {
    JsonDocument document;
    return lumalink::json::deserialize<polymorphic_model, model_spec>(document.as<JsonVariantConst>());
}