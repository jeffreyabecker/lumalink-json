# Getting Started And API

This document covers the shipped v1 entry points, setup requirements, and the smallest useful end-to-end example for `lumalink-json`.

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
template <typename Target, typename Spec, typename Binding = void>
json::expected<Target> deserialize(JsonVariantConst source, json::decode_options options = {});

template <typename Target, typename Spec, typename Binding = void>
json::expected<Target> deserialize(const JsonDocument& document, json::decode_options options = {});

template <typename Target, typename Spec, typename Binding = void>
json::expected<Target> deserialize(std::string_view source, JsonDocument& document, json::decode_options options = {});

template <typename Target, typename Spec, typename Binding = void>
json::expected<Target> deserialize(std::string_view source, json::decode_options options = {});

template <typename Spec, typename Source, typename Binding = void>
json::expected_void serialize(const Source& value, JsonVariant destination, json::encode_options options = {});

template <typename Spec, typename Source, typename Binding = void>
json::expected_void serialize(const Source& value, JsonDocument& document, json::encode_options options = {});

template <typename Spec>
json::expected_void generate_schema(JsonVariant destination);

template <typename Spec>
json::expected_void generate_schema(JsonDocument& document);
```

All fallible APIs return `std::expected` through `json::expected<T>` or `json::expected_void`.

`Binding` is optional. Leave it as `void` for aggregate auto-binding or when you still use `json::traits::object_fields`. Provide an explicit binding trait when you want to select member pointers at the call site.

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

## Next Topics

- Supported spec nodes and C++ type mappings are covered in [02-supported-spec-nodes.md](02-supported-spec-nodes.md).
- Supported option kinds are covered in [03-supported-options.md](03-supported-options.md).
- Binding modes, runtime behavior, schema generation, and customization points are covered in the remaining documents in this folder.