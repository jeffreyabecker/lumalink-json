# Schema Generation And Customization

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