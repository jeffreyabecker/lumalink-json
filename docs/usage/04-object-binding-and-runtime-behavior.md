# Object Binding And Runtime Behavior

## Object Binding Modes

### Aggregate Auto-Binding

If the target is a non-union, non-polymorphic aggregate, `spec::object` binds by position using vendored PFR. The field count in the spec must match the model arity.

```cpp
struct aggregate_endpoint {
    int id;
    std::string label;
    bool enabled;
};

using aggregate_endpoint_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"display_name", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"enabled", lumalink::json::spec::boolean<>>>;
```

The JSON keys come from the spec, so the second field above is serialized as `display_name` even though the member is named `label`.

### Explicit `object_fields` Registration

If the model is not auto-bindable, register member pointers explicitly.

```cpp
class manual_device_settings {
public:
    int id{};
    std::string label;
    bool enabled{};
};

namespace lumalink::json::traits {

template <>
struct object_fields<manual_device_settings> {
    static constexpr auto members = std::make_tuple(
        &manual_device_settings::id,
        &manual_device_settings::label,
        &manual_device_settings::enabled);
};

} // namespace lumalink::json::traits
```

The tuple arity must match the number of `spec::field` declarations, and each pointer must be a member object pointer compatible with the bound model type.

## Error Handling And Context Policies

Every failure returns `json::error`:

```cpp
auto result = lumalink::json::deserialize<int, lumalink::json::spec::integer<>>(R"("bad")");

if (!result.has_value()) {
    const auto& failure = result.error();

    if (failure.code == lumalink::json::error_code::unexpected_type) {
        // handle wrong JSON type
    }
}
```

Context collection is configurable:

```cpp
lumalink::json::decode_options options;
options.context = lumalink::json::context_policy::last;
options.nesting_limit = 32;

auto result = lumalink::json::deserialize<int, lumalink::json::spec::integer<>>("12", options);
```

Supported context policies:

1. `context_policy::full`
2. `context_policy::last`
3. `context_policy::none`

`decode_options.nesting_limit` only affects raw-string parse overloads, because that is where ArduinoJson parsing occurs.

## Validation Flow During Decode And Encode

For a given spec node, the runtime flow is:

1. Confirm the incoming or outgoing value matches the node kind.
2. Apply built-in constraints attached through options.
3. Apply any `opts::validator_func` hook.
4. Materialize the decoded C++ value or encoded JSON output only if all prior steps succeed.

This matters because a custom validator does not replace the library's built-in checks. For example, a string validator does not run if the JSON input is not a string in the first place.

### Nested Validation Context

Validation failures inside composite nodes report the active structural path according to the selected context policy.

```cpp
using reading_list_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<
        "values",
        lumalink::json::spec::array_of<
            lumalink::json::spec::integer<
                lumalink::json::opts::min_max_value<0, 10>>>>>;

auto result = lumalink::json::deserialize<some_model, reading_list_spec>(
    R"({"values":[1,2,99]})");
```

With `context_policy::full`, the failure context identifies the enclosing field and the indexed element that failed validation.

### Encode-Time Validation

Serialization is also a validation path. If a C++ value violates the bound spec, `serialize` returns an error and does not emit a partially validated payload.

```cpp
using small_list_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::min_max_elements<1, 3>>;

JsonDocument document;
auto encoded = lumalink::json::serialize<small_list_spec>(std::vector<int>{}, document);
```

The example above fails validation on encode because the container violates the declared minimum element count.

## Raw JSON Input And Lifetime Rules

If you decode from raw JSON into `std::string_view` or `const char*`, prefer the overload that accepts a caller-provided `JsonDocument` so the returned view stays valid as long as the document stays alive.

```cpp
JsonDocument document;
auto token = lumalink::json::deserialize<std::string_view, lumalink::json::spec::string<>>(
    R"("HELLO")",
    document);
```

If you use the convenience overload without a caller-provided document, the temporary `JsonDocument` is destroyed before the function returns, so view-based string targets are not appropriate there. Use `std::string` for owned results in that case.

## Encoding Into Existing JSON Storage

If you already have a `JsonDocument` or object/array slot, serialize into that destination directly.

```cpp
JsonDocument document;
JsonObject root = document.to<JsonObject>();
JsonVariant payload = root["payload"].to<JsonVariant>();

lumalink::json::serialize<lumalink::json::spec::integer<>>(17, payload);
```

Likewise, schema generation can target an existing destination variant:

```cpp
JsonDocument document;
JsonVariant schema_slot = document.to<JsonVariant>();

lumalink::json::generate_schema<lumalink::json::spec::integer<>>(schema_slot);
```