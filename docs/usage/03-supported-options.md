# Supported Options

This document covers spec-attached validation and metadata options. Some options only affect error context or schema output, while others participate directly in decode-time and encode-time validation.

## Validation Paths At A Glance

For the shipped v1 surface, validation can happen in these places:

1. Node-kind checks, such as requiring a JSON string for `spec::string<>` or a JSON array for `spec::array_of<>`.
2. Built-in option checks, such as `min_max_value`, `min_max_elements`, `pattern`, and `not_empty`.
3. User-supplied checks through `opts::validator_func`.

For decode, built-in structural and option checks run before `opts::validator_func`.
For encode, the library validates the outgoing C++ value against the spec before materializing JSON output.

If a validation step fails, the result carries `json::error` with the active context path and an error code that reflects which path failed. Structural mismatches report errors such as `unexpected_type`, while built-in and custom validation paths can report `value_out_of_range`, `pattern_mismatch`, or `validation_failed` depending on the failing rule.

## `opts::name`

`opts::name` provides a logical name that appears in error context.

```cpp
using named_speed_spec = lumalink::json::spec::integer<
    lumalink::json::opts::name<"motor_speed">>;
```

## `opts::min_max_value`

`opts::min_max_value<Min, Max>` applies to scalar numeric nodes. You can also pass an optional third template argument to override the default failure message.

```cpp
using bounded_speed_spec = lumalink::json::spec::integer<
    lumalink::json::opts::min_max_value<10, 20>>;

using bounded_gain_spec = lumalink::json::spec::number<
    lumalink::json::opts::min_max_value<0.0, 1.0>>;

using named_range_spec = lumalink::json::spec::integer<
    lumalink::json::opts::min_max_value<10, 20, "speed must be between 10 and 20">>;
```

Decode rejects JSON numbers outside the declared range. Encode also rejects outgoing C++ values outside the range instead of emitting invalid JSON.

## `opts::min_max_elements`

`opts::min_max_elements<Min, Max>` applies to `spec::array_of`. You can also pass an optional third template argument to override the default failure message.

```cpp
using bounded_list_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::min_max_elements<1, 3>>;

using named_list_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::min_max_elements<1, 3, "history must contain between 1 and 3 items">>;
```

This check applies symmetrically on decode and encode. An empty input array fails `min_max_elements<1, 3>`, and so does serializing an empty container for the same spec.

## `opts::pattern`

`opts::pattern<Predicate>` validates strings at runtime. If you also want JSON Schema output to include a `pattern` keyword, pass a schema literal as the second template argument. An optional third template argument overrides the default failure message.

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

using named_pattern_spec = lumalink::json::spec::string<
    lumalink::json::opts::pattern<is_uppercase_token, "^[A-Z]+$", "token must be uppercase">>;
```

The predicate drives runtime validation. The optional string literal only affects schema generation.

```cpp
auto decoded = lumalink::json::deserialize<std::string, schema_projected_pattern_spec>(R"("READY")");
auto rejected = lumalink::json::deserialize<std::string, schema_projected_pattern_spec>(R"("Ready")");
```

In the second call, the JSON type is correct, but validation still fails because the predicate rejects the content.

## `opts::not_empty`

`opts::not_empty` enforces that the validated value is not empty. It works for values that expose `empty()` or `size()`, and the string path supports string-like values through `std::string_view`. An optional template argument overrides the default failure message.

```cpp
using non_empty_name_spec = lumalink::json::spec::string<
    lumalink::json::opts::not_empty<"segment key is required">>;

using non_empty_runs_spec = lumalink::json::spec::array_of<
    lumalink::json::spec::integer<>,
    lumalink::json::opts::not_empty<"runs must not be empty">>;
```

For schema generation, `opts::not_empty` projects to `minLength: 1` on strings and `minItems: 1` on arrays.

## Built-In Custom Messages

The built-in validation options can carry their own failure messages when you want declarative constraints without switching to `opts::validator_func`.

```cpp
constexpr bool is_uppercase_code(std::string_view value) {
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

using segment_key_spec = lumalink::json::spec::string<
    lumalink::json::opts::not_empty<"segment.segmentKey is required">,
    lumalink::json::opts::pattern<
        is_uppercase_code,
        "^[A-Z]+$",
        "segment.segmentKey must be uppercase">>;

using physical_length_spec = lumalink::json::spec::integer<
    lumalink::json::opts::min_max_value<1, 4096, "segment.pixelLength must be at least 1">>;
```

This keeps the spec declarative while still producing domain-specific messages for the built-in validation paths.

## `opts::validator_func`

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

using small_spec = lumalink::json::spec::integer<
    lumalink::json::opts::validator_func<is_small>>;
```

## Composing Built-In And Custom Validation

Options compose on the same spec node, so you can layer built-in constraints with project-specific rules.

```cpp
using even_percent_spec = lumalink::json::spec::integer<
    lumalink::json::opts::min_max_value<0, 100>,
    lumalink::json::opts::validator_func<is_even>>;

auto ok = lumalink::json::deserialize<int, even_percent_spec>("42");
auto out_of_range = lumalink::json::deserialize<int, even_percent_spec>("101");
auto odd_value = lumalink::json::deserialize<int, even_percent_spec>("41");
```

`out_of_range` fails the built-in range check before the custom validator runs. `odd_value` passes the built-in range check and then fails the custom validator.

## Validation Failure Inspection

Validation failures are ordinary `json::error` results.

```cpp
auto result = lumalink::json::deserialize<int, even_percent_spec>("41");

if (!result.has_value()) {
    const auto& failure = result.error();

    if (failure.code == lumalink::json::error_code::validation_failed) {
        // inspect failure.context and failure.message
    }
}
```

## Common Validation Error Shapes

The exact error code depends on which validation path rejects the value:

1. `min_max_value` failures typically surface as `value_out_of_range`.
2. `pattern` failures surface as `pattern_mismatch`.
3. `not_empty` failures surface as `validation_failed`.
4. `opts::validator_func` failures commonly surface as `validation_failed`, unless the validator returns a richer `json::error` of its own.
5. Node-kind mismatches, such as passing a JSON string to `spec::integer<>`, surface as structural errors such as `unexpected_type` rather than option-validation errors.