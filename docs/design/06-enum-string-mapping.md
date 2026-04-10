# Enum String Mapping Pattern

## Objective

Define a stable and explicit pattern for converting between JSON string tokens and strongly typed C++ enums in both decode and encode paths.

All fallible operations in this pattern return `std::expected`.

## Canonical spec node

Use a dedicated node for enum string values:

1. `spec::enum_string<E, Opts>`

Behavior:

1. decode: JSON string token -> enum value
2. encode: enum value -> JSON string token

## Preferred mapping path

The preferred user-facing mapping path is a CRTP codec type that exposes a `values` table.

Example shape:

```cpp
template <typename Derived, typename E>
struct json::enum_codec {
    using enum_type = E;
};
```

Enum codec example:

```cpp
enum class mode {
    off,
    auto_mode,
    manual
};

struct mode_codec : json::enum_codec<mode_codec, mode> {
    static constexpr std::array<json::traits::enum_mapping_entry<mode>, 3> values{{
        {"off", mode::off},
        {"auto", mode::auto_mode},
        {"manual", mode::manual},
    }};
};

using mode_spec = json::spec::enum_string<mode_codec>;
```

## Legacy compatibility path

The original trait specialization path remains supported.

```cpp
template <>
struct json::traits::enum_strings<mode> {
    static constexpr std::array<json::traits::enum_mapping_entry<mode>, 3> values{{
        {"off", mode::off},
        {"auto", mode::auto_mode},
        {"manual", mode::manual},
    }};
};

using legacy_mode_spec = json::spec::enum_string<mode>;
```

## Decode algorithm contract

1. verify JSON value is a string
2. lookup token in the resolved mapping table
3. if found, return enum value
4. if not found, return `std::unexpected(json::error{enum_string_unknown, ...})`

Return type:

- `std::expected<E, json::error>`

## Encode algorithm contract

1. lookup enum value in the resolved mapping table
2. if found, emit the mapped token into the destination JSON value
3. if not found, return `std::unexpected(json::error{enum_value_unmapped, ...})`

Return type:

- `std::expected<void, json::error>`

## Validation and options

`enum_string<E, Opts>` accepts normal node options:

1. `opts::name`
2. `opts::validator_func`

Validator execution order:

1. decode token to enum
2. run validator on enum value
3. return validation error through `std::expected` if validator fails

## Determinism rules

1. each string token maps to exactly one enum value
2. each encoded enum value must map to exactly one canonical token
3. duplicate tokens are compile-time errors
4. unknown tokens never coerce to default enum values

## Case handling policy

Default policy is case-sensitive exact token matching.

Optional future extension:

1. `opts::token_compare<Policy>`
   - allows case-insensitive or locale-specific comparison

## Error context guidance

When enum mapping fails, context should include:

1. field key when in object field
2. logical node name if supplied by `opts::name`
3. node kind `enum_string`

## Testing requirements

1. round-trip success for every mapped enum entry
2. unknown token decode returns `enum_string_unknown`
3. unmapped enum encode returns `enum_value_unmapped`
4. duplicate token map is rejected at compile time
5. validator failure on resolved enum is surfaced with context
2. logical node name if supplied by opts::name
3. node kind enum_string

## Testing requirements

1. round-trip success for every mapped enum entry
2. unknown token decode returns enum_string_unknown
3. unmapped enum encode returns enum_value_unmapped
4. duplicate token map is rejected at compile time
5. validator failure on resolved enum is surfaced with context
