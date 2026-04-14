# Supported Spec Nodes

The shipped v1 node surface maps to the following C++ categories.

| Spec node | Supported C++ type(s) |
| --- | --- |
| `spec::null<>` | `std::nullptr_t` |
| `spec::boolean<>` | `bool` |
| `spec::integer<>` | any integral type except `bool` |
| `spec::number<>` | any floating-point type |
| `spec::string<>` | `std::string`, `std::string_view`, `const char*` |
| `spec::enum_string<E>` | enum `E` with a `json::enum_codec` type, or `json::traits::enum_strings<E>` for legacy compatibility |
| `spec::any<>` | `JsonVariantConst` |
| `spec::with_codec<InnerSpec, Codec>` | any `T` that `Codec` explicitly supports |
| `spec::object<...>` | aggregate class/struct models, or models with `json::traits::object_fields<Model>` |
| `spec::optional<T>` | `std::optional<T>` |
| `spec::array_of<T>` | append containers with `value_type`, `push_back`, `size`, `begin`, `end` |
| `spec::tuple<...>` | `std::tuple<...>` with matching arity |
| `spec::one_of<...>` | `std::variant<...>` with matching order and arity |
| `spec::error` | `json::error` |

## Null, Boolean, Integer, Number

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

## Strings

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

## Enum Strings

```cpp
enum class mode : unsigned char {
    standby,
    active,
    maintenance,
};

struct mode_codec : lumalink::json::enum_codec<mode_codec, mode> {
    static constexpr std::array<lumalink::json::traits::enum_mapping_entry<mode>, 3> values{{
        {"standby", mode::standby},
        {"active", mode::active},
        {"maintenance", mode::maintenance},
    }};
};

using mode_spec = lumalink::json::spec::enum_string<mode_codec>;

auto decoded = lumalink::json::deserialize<mode, mode_spec>(R"("active")");

JsonDocument encoded;
lumalink::json::serialize<mode_spec>(mode::maintenance, encoded);
```

This is the preferred authoring path because it keeps the string mapping next to a concrete codec type in user code rather than requiring a specialization in `lumalink::json::traits`.

`spec::enum_string<mode>` with `json::traits::enum_strings<mode>` remains available for legacy compatibility, but new bindings should prefer a dedicated codec type.

## Any

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

## Objects

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

## Optional Fields

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

## Arrays

`spec::array_of<T>` works with append containers such as `std::vector<T>`.

```cpp
using readings_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::min_max_elements<1, 4>>;

auto readings = lumalink::json::deserialize<std::vector<int>, readings_spec>("[10,20,30]");

JsonDocument encoded;
lumalink::json::serialize<readings_spec>(std::vector<int>{1, 2, 3}, encoded);
```

## Tuples

`spec::tuple` maps to `std::tuple` and requires exact positional arity.

```cpp
using triple_spec = lumalink::json::spec::tuple<
    lumalink::json::spec::integer<>,
    lumalink::json::spec::string<>,
    lumalink::json::spec::boolean<>>;

using triple_value = std::tuple<int, std::string, bool>;

auto triple = lumalink::json::deserialize<triple_value, triple_spec>("[7,\"node\",true]");
```

## Variants With `one_of`

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

## Built-In Error Payload

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