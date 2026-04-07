# Error Model Using std::expected

## Core decision

All fallible API surfaces and internal node handlers return std::expected.
No project-specific result wrapper type is defined.

## Canonical aliases

Recommended aliases:

- using expected_void = std::expected<void, json::error>;
- template <typename T>
  using expected = std::expected<T, json::error>;

These are aliases only, not wrapper classes.

## Error payload

json::error contains:

1. error_code code
2. compact context stack
3. optional short message view
4. optional backend status mapping info

## Error code taxonomy

1. structural
   - missing_field
   - unexpected_type
   - array_size_mismatch
2. validation
   - validation_failed
   - value_out_of_range
   - pattern_mismatch
   - enum_string_unknown
   - enum_value_unmapped
3. backend and parse
   - empty_input
   - incomplete_input
   - invalid_input
   - no_memory
   - too_deep

## Context model

Each context entry may carry:

1. node kind
2. logical name option
3. object field key when applicable

Policy controls retained depth:

1. full
2. last
3. none

## Propagation pattern

1. each node returns std::expected<node_value, json::error>
2. caller checks has_value
3. on failure, caller augments context and returns unexpected(error)
4. public API returns first failure with full retained context per policy

## Public API contract

1. deserialize returns std::expected<T, json::error>
2. serialize returns std::expected<void, json::error>
3. no throwing behavior is required for callers

## C++ version note

std::expected is standardized in C++23. This design therefore requires a toolchain that provides std::expected in <expected>.
