# Usage Guide

This document covers the shipped v1 usage surface of `lumalink-json`: supported spec nodes, supported option kinds, entry-point overloads, extension hooks, schema generation, and the main constraints you need to account for when binding JSON to C++ models.

## Include And Setup

`lumalink-json` is header-only. Include the umbrella header:

```cpp
#include <LumaLinkJson.h>
```

You will typically also need standard library headers for the C++ types you bind:

```cpp
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>
```

All examples below assume ArduinoJson 7 is available and `JsonDocument` is the backing storage for parsed or encoded JSON.

## Public API

The public entry points are:

```cpp
template <typename Target, typename Spec>
json::expected<Target> deserialize(JsonVariantConst source, json::decode_options options = {});

template <typename Target, typename Spec>
json::expected<Target> deserialize(const JsonDocument& document, json::decode_options options = {});

template <typename Target, typename Spec>
json::expected<Target> deserialize(std::string_view source, JsonDocument& document, json::decode_options options = {});

template <typename Target, typename Spec>
json::expected<Target> deserialize(std::string_view source, json::decode_options options = {});

template <typename Spec, typename Source>
json::expected_void serialize(const Source& value, JsonVariant destination, json::encode_options options = {});

template <typename Spec, typename Source>
json::expected_void serialize(const Source& value, JsonDocument& document, json::encode_options options = {});

template <typename Spec>
json::expected_void generate_schema(JsonVariant destination);

template <typename Spec>
json::expected_void generate_schema(JsonDocument& document);
```

All fallible APIs return `std::expected` through `json::expected<T>` or `json::expected_void`.

## Quick Start

This is the smallest useful end-to-end example with a structured model and `spec::object` binding:

```cpp
#include <LumaLinkJson.h>

#include <string>

struct device_config {
    int id;
    std::string label;
    bool enabled;
};

using device_config_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"label", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"enabled", lumalink::json::spec::boolean<>>>;

void example() {
    auto decoded = lumalink::json::deserialize<device_config, device_config_spec>(
        R"({"id":7,"label":"desk","enabled":true})");

    if (!decoded.has_value()) {
        const auto& failure = decoded.error();
        return;
    }

    JsonDocument encoded;
    auto encoded_result = lumalink::json::serialize<device_config_spec>(*decoded, encoded);
    if (!encoded_result.has_value()) {
        return;
    }
}
```

## Supported Spec Nodes

The shipped v1 node surface maps to the following C++ categories.

| Spec node | Supported C++ type(s) |
| --- | --- |
| `spec::null<>` | `std::nullptr_t` |
| `spec::boolean<>` | `bool` |
| `spec::integer<>` | any integral type except `bool` |
| `spec::number<>` | any floating-point type |
| `spec::string<>` | `std::string`, `std::string_view`, `const char*` |
| `spec::enum_string<E>` | enum `E` with `json::traits::enum_strings<E>` |
| `spec::any<>` | `JsonVariantConst` |
| `spec::with_codec<InnerSpec, Codec>` | any `T` that `Codec` explicitly supports |
| `spec::object<...>` | aggregate class/struct models, or models with `json::traits::object_fields<Model>` |
| `spec::optional<T>` | `std::optional<T>` |
| `spec::array_of<T>` | append containers with `value_type`, `push_back`, `size`, `begin`, `end` |
| `spec::tuple<...>` | `std::tuple<...>` with matching arity |
| `spec::one_of<...>` | `std::variant<...>` with matching order and arity |
| `spec::error` | `json::error` |

### Null, Boolean, Integer, Number

```cpp
using null_spec = lumalink::json::spec::null<>;
using enabled_spec = lumalink::json::spec::boolean<>;
using percent_spec = lumalink::json::spec::integer<lumalink::json::opts::min_max_value<0, 100>>;
using gain_spec = lumalink::json::spec::number<lumalink::json::opts::min_max_value<0.0, 1.0>>;

auto null_value = lumalink::json::deserialize<std::nullptr_t, null_spec>("null");
auto enabled = lumalink::json::deserialize<bool, enabled_spec>("true");
auto percent = lumalink::json::deserialize<int, percent_spec>("42");
auto gain = lumalink::json::deserialize<float, gain_spec>("0.75");
```

### Strings

```cpp
constexpr bool is_uppercase(std::string_view value) {
    if (value.empty()) {
        return false;
    }

    for (char ch : value) {
        if (ch < 'A' || ch > 'Z') {
            return false;
        }
    }

    return true;
}

using token_spec = lumalink::json::spec::string<
    lumalink::json::opts::pattern<is_uppercase, "^[A-Z]+$">>;

auto owned = lumalink::json::deserialize<std::string, token_spec>(R"("READY")");

JsonDocument backing_doc;
auto view = lumalink::json::deserialize<std::string_view, token_spec>(
    R"("READY")",
    backing_doc);

auto c_string = lumalink::json::deserialize<const char*, token_spec>(
    R"("READY")",
    backing_doc);
```

Use `std::string` if you want an owned value. Use `std::string_view` or `const char*` only when the backing `JsonDocument` will outlive the decoded view.

### Enum Strings

```cpp
enum class mode : unsigned char {
    standby,
    active,
    maintenance,
};

namespace lumalink::json::traits {

template <>
struct enum_strings<mode> {
    static constexpr std::array<enum_mapping_entry<mode>, 3> values{{
        {"standby", mode::standby},
        {"active", mode::active},
        {"maintenance", mode::maintenance},
    }};
};

} // namespace lumalink::json::traits

using mode_spec = lumalink::json::spec::enum_string<mode>;

auto decoded = lumalink::json::deserialize<mode, mode_spec>(R"("active")");

JsonDocument encoded;
lumalink::json::serialize<mode_spec>(mode::maintenance, encoded);
```

### Any

`spec::any<>` passes through a JSON subtree as `JsonVariantConst`.

```cpp
using raw_payload_spec = lumalink::json::spec::any<>;

JsonDocument document;
deserializeJson(document, R"({"kind":"event","count":3})");

auto payload = lumalink::json::deserialize<JsonVariantConst, raw_payload_spec>(document);
```

If you need an erased application type such as `std::any`, wrap `spec::any<>` with an explicit codec using `spec::with_codec<InnerSpec, Codec>`. The codec owns the runtime safety rules and decides which contained C++ types are valid.

```cpp
#include <any>

struct tagged_any_codec {
    template <typename InnerSpec, typename T>
    static lumalink::json::expected<T> decode(
        JsonVariantConst source,
        const lumalink::json::decode_state& state);

    template <typename InnerSpec, typename T>
    static lumalink::json::expected_void encode(
        const T& value,
        JsonVariant destination,
        const lumalink::json::encode_state& state);
};

using payload_spec = lumalink::json::spec::with_codec<
    lumalink::json::spec::any<>,
    tagged_any_codec>;

auto decoded = lumalink::json::deserialize<std::any, payload_spec>(document);
```

For `std::any`, the codec should reject unsupported or ambiguous payloads explicitly. The framework does not infer a concrete contained type on its own.

### Objects

`spec::object` binds using spec-declared keys. The spec field names, not the C++ member names, define the JSON wire contract.

```cpp
struct endpoint {
    int id;
    std::string display_name;
    bool enabled;
};

using endpoint_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<"display_name", lumalink::json::spec::string<>>,
    lumalink::json::spec::field<"enabled", lumalink::json::spec::boolean<>>>;

auto endpoint_value = lumalink::json::deserialize<endpoint, endpoint_spec>(
    R"({"id":11,"display_name":"Panel","enabled":true})");
```

### Optional Fields

`spec::optional<T>` maps to `std::optional<T>`. In object binding, missing optional fields reset the member instead of failing.

```cpp
struct counter_model {
    std::optional<int> count;
};

using counter_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"count", lumalink::json::spec::optional<lumalink::json::spec::integer<>>>>;

auto missing = lumalink::json::deserialize<counter_model, counter_spec>(R"({})");
auto present = lumalink::json::deserialize<counter_model, counter_spec>(R"({"count":12})");
```

### Arrays

`spec::array_of<T>` works with append containers such as `std::vector<T>`.

```cpp
using readings_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::min_max_elements<1, 4>>;

auto readings = lumalink::json::deserialize<std::vector<int>, readings_spec>("[10,20,30]");

JsonDocument encoded;
lumalink::json::serialize<readings_spec>(std::vector<int>{1, 2, 3}, encoded);
```

### Tuples

`spec::tuple` maps to `std::tuple` and requires exact positional arity.

### Built-In Error Payload

`spec::error` provides a predefined wire shape for `json::error`, including the mapped `error_code`, the active context entries, the optional message, and the optional backend status.

```cpp
lumalink::json::error failure{
        lumalink::json::error_code::unexpected_type,
        {},
        "expected object",
        17,
};

JsonDocument document;
lumalink::json::serialize<lumalink::json::spec::error>(failure, document);
```

The encoded JSON shape is:

```json
{
    "code": "unexpected_type",
    "context": [],
    "message": "expected object",
    "backend_status": 17
}
```

```cpp
using triple_spec = lumalink::json::spec::tuple<
    lumalink::json::spec::integer<>,
    lumalink::json::spec::string<>,
    lumalink::json::spec::boolean<>>;

using triple_value = std::tuple<int, std::string, bool>;

auto triple = lumalink::json::deserialize<triple_value, triple_spec>("[7,\"node\",true]");
```

### Variants With `one_of`

`spec::one_of` maps to `std::variant`. Alternative order matters for both the spec and the variant type.

```cpp
using identifier_spec = lumalink::json::spec::one_of<
    lumalink::json::spec::integer<>,
    lumalink::json::spec::string<>>;

using identifier_value = std::variant<int, std::string>;

auto as_integer = lumalink::json::deserialize<identifier_value, identifier_spec>("42");
auto as_string = lumalink::json::deserialize<identifier_value, identifier_spec>(R"("fallback")");
```

Decode tries alternatives in declaration order and returns the first fully successful match.

## Supported Options

### `opts::name`

`opts::name` provides a logical name that appears in error context.

```cpp
using named_speed_spec = lumalink::json::spec::integer<
    lumalink::json::opts::name<"motor_speed">>;
```

### `opts::min_max_value`

`opts::min_max_value<Min, Max>` applies to scalar numeric nodes:

```cpp
using bounded_speed_spec = lumalink::json::spec::integer<
    lumalink::json::opts::min_max_value<10, 20>>;

using bounded_gain_spec = lumalink::json::spec::number<
    lumalink::json::opts::min_max_value<0.0, 1.0>>;
```

### `opts::min_max_elements`

`opts::min_max_elements<Min, Max>` applies to `spec::array_of`:

```cpp
using bounded_list_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::min_max_elements<1, 3>>;
```

### `opts::pattern`

`opts::pattern<Predicate>` validates strings at runtime. If you also want JSON Schema output to include a `pattern` keyword, pass a schema literal as the second template argument.

```cpp
constexpr bool is_uppercase_token(std::string_view value) {
    if (value.empty()) {
        return false;
    }

    for (char ch : value) {
        if (ch < 'A' || ch > 'Z') {
            return false;
        }
    }

    return true;
}

using runtime_only_pattern_spec = lumalink::json::spec::string<
    lumalink::json::opts::pattern<is_uppercase_token>>;

using schema_projected_pattern_spec = lumalink::json::spec::string<
    lumalink::json::opts::pattern<is_uppercase_token, "^[A-Z]+$">>;
```

### `opts::validator_func`

Validators run after built-in decode checks succeed, and they also run during encode. Supported validator return types are:

1. `bool`
2. `std::expected<void, json::error_code>`
3. `json::expected_void`

Examples:

```cpp
bool is_even(const int value) {
    return (value % 2) == 0;
}

std::expected<void, lumalink::json::error_code> is_nonzero(const int value) {
    if (value != 0) {
        return {};
    }

    return std::unexpected(lumalink::json::error_code::validation_failed);
}

lumalink::json::expected_void is_small(const int value) {
    if (value <= 100) {
        return {};
    }

    return std::unexpected(lumalink::json::error{
        lumalink::json::error_code::validation_failed,
        {},
        "value must be <= 100"
    });
}

using even_spec = lumalink::json::spec::integer<
    lumalink::json::opts::validator_func<is_even>>;

using nonzero_spec = lumalink::json::spec::integer<
    lumalink::json::opts::validator_func<is_nonzero>>;

using small_spec = lumalink::json::spec::integer<
    lumalink::json::opts::validator_func<is_small>>;
```

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

## Schema Generation

The shipped v1 schema emitter supports the current node surface and supported option projections.

```cpp
using endpoint_schema_spec = lumalink::json::spec::object<
    lumalink::json::spec::field<"id", lumalink::json::spec::integer<>>,
    lumalink::json::spec::field<
        "token",
        lumalink::json::spec::string<
            lumalink::json::opts::pattern<is_uppercase, "^[A-Z]+$">>>,
    lumalink::json::spec::field<
        "count",
        lumalink::json::spec::optional<
            lumalink::json::spec::integer<
                lumalink::json::opts::min_max_value<0, 99>>>>>;

JsonDocument schema;
auto schema_result = lumalink::json::generate_schema<endpoint_schema_spec>(schema);
```

Supported schema projections include:

1. scalar `type`
2. `object` `properties` and `required`
3. `array_of` `items`, `minItems`, and `maxItems`
4. `tuple` `prefixItems`, `minItems`, and `maxItems`
5. `optional` and `one_of` as `anyOf`
6. `enum_string` as string `enum`
7. `min_max_value` as `minimum` and `maximum`
8. `pattern` only when the option carries a schema literal

## Custom Decoder And Encoder Specializations

You can specialize `json::decoder<Spec, T>` and `json::encoder<Spec, T>` for non-standard wire formats.

```cpp
struct project_token {
    int value{};
};

namespace lumalink::json {

template <>
struct decoder<spec::string<>, project_token> {
    static expected<project_token> decode(JsonVariantConst source, const decode_state& state) {
        if (!source.is<const char*>()) {
            return std::unexpected(detail::make_error<spec::string<>>(
                error_code::unexpected_type,
                state.context,
                "expected token string"));
        }

        const char* raw = source.as<const char*>();
        if (raw == nullptr || std::string_view{raw}.size() < 4U) {
            return std::unexpected(detail::make_error<spec::string<>>(
                error_code::validation_failed,
                state.context,
                "invalid project token"));
        }

        return project_token{42};
    }
};

template <>
struct encoder<spec::string<>, project_token> {
    static expected_void encode(const project_token& value, JsonVariant destination, const encode_state& state) {
        if (value.value < 0) {
            return std::unexpected(detail::make_error<spec::string<>>(
                error_code::validation_failed,
                state.context,
                "project token must be non-negative"));
        }

        destination.set(std::string("TK-") + std::to_string(value.value));
        return {};
    }
};

} // namespace lumalink::json
```

This is the supported v1 customization point for project-specific value codecs.

## Explicit Codec Wrappers

You can also bind a spec node to an explicit codec type with `spec::with_codec<InnerSpec, Codec>`. This is useful when you want the spec itself to select the codec instead of relying on a global `json::decoder<Spec, T>` or `json::encoder<Spec, T>` specialization.

The wrapper delegates to these codec entry points:

```cpp
template <typename InnerSpec, typename T>
static json::expected<T> Codec::decode(
    JsonVariantConst source,
    const json::decode_state& state);

template <typename InnerSpec, typename T>
static json::expected_void Codec::encode(
    const T& value,
    JsonVariant destination,
    const json::encode_state& state);

template <typename InnerSpec>
static json::expected_void Codec::emit_schema(JsonVariant destination);
```

`emit_schema` is optional. If the codec does not provide it, schema generation falls back to `InnerSpec`.

Example:

```cpp
struct tagged_any_codec {
    template <typename InnerSpec, typename T>
    static lumalink::json::expected<T> decode(
        JsonVariantConst source,
        const lumalink::json::decode_state& state);

    template <typename InnerSpec, typename T>
    static lumalink::json::expected_void encode(
        const T& value,
        JsonVariant destination,
        const lumalink::json::encode_state& state);

    template <typename InnerSpec>
    static lumalink::json::expected_void emit_schema(JsonVariant destination);
};

using payload_spec = lumalink::json::spec::with_codec<
    lumalink::json::spec::any<>,
    tagged_any_codec>;
```

`InnerSpec` remains the source of error-context metadata. For schema generation, the wrapper uses `Codec::emit_schema` when present and otherwise falls back to `InnerSpec`. For example, wrapping `spec::string<>` without a schema hook still emits a string schema.

## Constraints And Gotchas

1. `spec::object` field count must match the bound model arity or explicit `object_fields` tuple arity.
2. `spec::object` auto-binding supports aggregates only; unions and polymorphic types require explicit trait registration and some shapes are still rejected.
3. The current PFR-backed auto-binding arity limit is 200 fields.
4. Optional object fields require `std::optional` members on the bound C++ type.
5. `spec::tuple` requires exact tuple length on decode.
6. `spec::one_of` requires `std::variant` order and arity to match the spec declaration exactly.
7. `spec::array_of` requires an append container; fixed-size arrays are not part of the shipped v1 surface.
8. `spec::any<>` is a pass-through JSON node, so the bound type is `JsonVariantConst`.
9. `spec::with_codec<InnerSpec, Codec>` requires `Codec::decode` and `Codec::encode` with the exact signatures shown above.
10. `Codec::emit_schema` is optional, but if you provide it the exact return type must be `json::expected_void`.
11. If you use `spec::with_codec<spec::any<>, Codec>` for `std::any`, the codec is responsible for all runtime type discrimination and for rejecting unsupported payloads safely.
12. `opts::validator_func` accepts only `bool`, `std::expected<void, json::error_code>`, or `json::expected_void` return types.
13. Runtime-only `opts::pattern<Predicate>` validation does not automatically imply a schema `pattern`; provide the schema literal explicitly if you need schema projection.
14. `std::string_view` and `const char*` decode targets depend on the lifetime of the backing `JsonDocument`.

## Where To Go Next

If you need implementation or design details beyond this usage guide, the design set starts at [docs/design/README.md](docs/design/README.md).