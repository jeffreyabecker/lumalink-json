# Upgrading To v0.11.0

This release adds an explicit call-site object binding path for `spec::object`.

Use this when a downstream consumer currently relies on `json::traits::object_fields<Model>` and wants the binding to live next to the `serialize` and `deserialize` calls instead of in a namespace specialization.

## From

```cpp
class manual_device_settings {
public:
    int id{};
    std::string label;
    bool enabled{};
};

using manual_device_settings_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"display_name", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"enabled", lumalink::json::spec::boolean<>>>;

namespace lumalink::json::traits {

template <>
struct object_fields<manual_device_settings> {
    static constexpr auto members = std::make_tuple(
        &manual_device_settings::id,
        &manual_device_settings::label,
        &manual_device_settings::enabled);
};

} // namespace lumalink::json::traits

auto decoded = lumalink::json::deserialize<manual_device_settings, manual_device_settings_spec>(source);

JsonDocument encoded;
auto encoded_result = lumalink::json::serialize<manual_device_settings_spec>(value, encoded);
```

## To

```cpp
class manual_device_settings {
public:
    int id{};
    std::string label;
    bool enabled{};
};

using manual_device_settings_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"display_name", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"enabled", lumalink::json::spec::boolean<>>>;

struct manual_device_settings_binding {
    using model_type = manual_device_settings;

    static constexpr auto members = std::make_tuple(
        &manual_device_settings::id,
        &manual_device_settings::label,
        &manual_device_settings::enabled);
};

auto decoded = lumalink::json::deserialize<
    manual_device_settings,
    manual_device_settings_spec,
    manual_device_settings_binding>(source);

JsonDocument encoded;
auto encoded_result = lumalink::json::serialize<
    manual_device_settings_spec,
    manual_device_settings,
    manual_device_settings_binding>(value, encoded);
```

## Prompt-Friendly Summary

Replace each `json::traits::object_fields<Model>` specialization with a local binding struct:

- Keep the same `members` tuple.
- Add `using model_type = Model;`.
- Pass that binding as the third template argument to `deserialize<T, Spec, Binding>(...)`.
- Pass that binding as the third template argument to `serialize<Spec, Source, Binding>(...)`.
- Leave aggregate auto-bound models unchanged.

## Notes

- `json::traits::object_fields<Model>` still works as a fallback, but the new call-site form is the preferred migration target.
- The binding tuple still has the same rules: same arity as the `spec::field` list and only member object pointers compatible with the bound model.
