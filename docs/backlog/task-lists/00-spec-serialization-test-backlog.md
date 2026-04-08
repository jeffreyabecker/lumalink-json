# Spec Serialization Test Backlog

## Purpose

This backlog defines the test work needed to cover the full supported surface of the spec-based JSON serialization and deserialization design.

The target runtime harness is PlatformIO native tests using Unity.
Compile-time negative coverage should be implemented as separate build-only checks, because several supported failure modes are specified as static assertions or template-instantiation failures rather than runtime errors.

## Test Strategy

1. Runtime conformance tests
   - Use Unity under the native_tests PlatformIO environment.
   - Cover successful decode, successful encode, round-trip, and runtime error propagation.

2. Compile-time conformance tests
   - Add build-only cases for duplicate options, unsupported reflection targets, and other invalid template uses.
   - Treat compile failure as the expected outcome for these cases.

3. Shared test utilities
   - Add helpers for JSON document setup, decode/encode invocation, round-trip assertions, and structured error assertions.
   - Add helpers for checking error_code, node kind, field key, and context depth behavior.

## Phase 1: Harness Foundation

- TST-01 [todo] : Add a reusable native Unity fixture for decode and encode tests.
- TST-02 [todo] : Add assertion helpers for `std::expected` success and failure results.
- TST-03 [todo] : Add assertion helpers for `json::error_code` and error-context contents.
- TST-04 [todo] : Add a round-trip helper that encodes a model, reparses it, and decodes it back.
- TST-05 [todo] : Add a compile-only test harness for negative template and `static_assert` cases.
- TST-06 [todo] : Define naming conventions for runtime suites by spec node and behavior category.

## Phase 2: Scalar Node Coverage

- SCL-01 [todo] : Add `null` node tests for decode success from JSON `null`.
- SCL-02 [todo] : Add `null` node tests for rejecting non-null values.
- SCL-03 [todo] : Add `boolean` node tests for `true` and `false` decode and encode.
- SCL-04 [todo] : Add `boolean` node tests for wrong-type failures.
- SCL-05 [todo] : Add `integer` node tests for signed and unsigned target types.
- SCL-06 [todo] : Add `integer` node tests for boundary values and overflow or out-of-range failures.
- SCL-07 [todo] : Add `integer` node tests for `min_max_value` option behavior.
- SCL-08 [todo] : Add `number` node tests for `float` and `double` decode and encode.
- SCL-09 [todo] : Add `number` node tests that lock down accepted numeric input forms.
- SCL-10 [todo] : Add `string` node tests for empty, non-empty, and wrong-type values.
- SCL-11 [todo] : Add `string` node tests that lock down lifetime expectations for the supported string target types.
- SCL-12 [todo] : Add `string` node tests for `pattern` option behavior.
- SCL-13 [todo] : Add `enum_string` node tests for successful decode of every mapped token.
- SCL-14 [todo] : Add `enum_string` node tests for successful encode of every mapped enum value.
- SCL-15 [todo] : Add `enum_string` node tests for `enum_string_unknown` on unknown input tokens.
- SCL-16 [todo] : Add `enum_string` node tests for `enum_value_unmapped` on unmapped encode values.
- SCL-17 [todo] : Add `any` node tests for pass-through decode and encode behavior.

## Phase 3: Composite Node Coverage

- CMP-01 [todo] : Add `field` and `object` tests for required-field decode success.
- CMP-02 [todo] : Add `field` and `object` tests for `missing_field` failures.
- CMP-03 [todo] : Add `field` and `object` tests for nested field context propagation.
- CMP-04 [todo] : Add `object` tests for deterministic declaration-order traversal.
- CMP-05 [todo] : Add `optional` node tests for present values.
- CMP-06 [todo] : Add `optional` node tests for missing fields.
- CMP-07 [todo] : Add `optional` node tests for explicit JSON `null` values.
- CMP-08 [todo] : Add `array_of` tests for homogeneous container decode and encode.
- CMP-09 [todo] : Add `array_of` tests for per-element type failures with index-aware context.
- CMP-10 [todo] : Add `array_of` tests for `min_max_elements` behavior.
- CMP-11 [todo] : Add `tuple` tests for exact positional decode and encode.
- CMP-12 [todo] : Add `tuple` tests for `array_size_mismatch` failures.
- CMP-13 [todo] : Add `tuple` tests for per-position type mismatch context.
- CMP-14 [todo] : Add `one_of` tests for ordered alternative selection with first successful match.
- CMP-15 [todo] : Add `one_of` tests for all-alternatives-failed error reporting.
- CMP-16 [todo] : Add `one_of` tests that lock down the supported ambiguity behavior.

## Phase 4: Validator And Option Coverage

- VAL-01 [todo] : Add `validator_func` tests for boolean-return validators on success.
- VAL-02 [todo] : Add `validator_func` tests for boolean-return validators on failure.
- VAL-03 [todo] : Add `validator_func` tests for `std::expected`-return validators on success.
- VAL-04 [todo] : Add `validator_func` tests for `std::expected`-return validators on failure.
- VAL-05 [todo] : Add tests showing validators run after successful base decode.
- VAL-06 [todo] : Add tests showing validators run during encode when required by the design.
- VAL-07 [todo] : Add `opts::name` tests proving logical names appear in error context.
- VAL-08 [todo] : Add `min_max_elements` tests at lower and upper bounds.
- VAL-09 [todo] : Add `min_max_value` tests at lower and upper bounds.
- VAL-10 [todo] : Add `pattern` tests for accepted and rejected string values.
- VAL-11 [todo] : Add compile-fail tests for duplicate option categories on a single node.

## Phase 5: Error Model And Context Coverage

- ERR-01 [todo] : Add tests for every supported runtime `error_code`.
- ERR-02 [todo] : Add tests for `full` context policy.
- ERR-03 [todo] : Add tests for `last` context policy.
- ERR-04 [todo] : Add tests for `none` context policy.
- ERR-05 [todo] : Add tests proving first failure is returned without hidden recovery.
- ERR-06 [todo] : Add tests for nested `object -> array -> tuple -> scalar` failure context.
- ERR-07 [todo] : Add tests for enum mapping failures including node kind, field key, and logical name.
- ERR-08 [todo] : Add tests for `value_out_of_range`, `pattern_mismatch`, and `validation_failed` differentiation.

## Phase 6: Parse API And Backend Mapping Coverage

- BCK-01 [todo] : Add tests for deserialize from `JsonVariantConst`.
- BCK-02 [todo] : Add tests for deserialize from raw JSON input.
- BCK-03 [todo] : Add tests mapping empty input to `empty_input`.
- BCK-04 [todo] : Add tests mapping incomplete JSON input to `incomplete_input`.
- BCK-05 [todo] : Add tests mapping malformed JSON input to `invalid_input`.
- BCK-06 [todo] : Add tests mapping backend memory exhaustion to `no_memory` where practical.
- BCK-07 [todo] : Add tests mapping excessive nesting to `too_deep` where practical.

## Phase 7: Extensibility Coverage

- EXT-01 [todo] : Add tests for custom decoder specialization on a non-standard target type.
- EXT-02 [todo] : Add tests for custom encoder specialization on a non-standard source type.
- EXT-03 [todo] : Add tests for custom enum mapping traits on project-specific enums.
- EXT-04 [todo] : Add tests proving custom converter failures propagate as `std::expected` errors.
- EXT-05 [todo] : Add tests for custom `one_of` discrimination hooks if that extension point ships in v1.

## Phase 8: Reflection And Binding Coverage

- RFL-01 [todo] : Add runtime tests for `pfr`-backed object binding on supported simple aggregates.
- RFL-02 [todo] : Add tests proving spec field keys, not struct field names, drive object mapping.
- RFL-03 [todo] : Add tests for explicit field-trait fallback on types that are not auto-reflectable.
- RFL-04 [todo] : Add compile-fail tests for non-aggregate model types on the auto-reflection path.
- RFL-05 [todo] : Add compile-fail tests for polymorphic model types on the auto-reflection path.
- RFL-06 [todo] : Add compile-fail tests for union model types on the auto-reflection path.
- RFL-07 [todo] : Add compile-fail tests for inherited aggregate layouts on the auto-reflection path.
- RFL-08 [todo] : Add compile-fail or guard tests for aggregates exceeding the supported `pfr` arity limit.

## Phase 9: Serialization Round-Trip Coverage

- RT-01 [todo] : Add round-trip tests for scalar nodes.
- RT-02 [todo] : Add round-trip tests for nested objects.
- RT-03 [todo] : Add round-trip tests for arrays, tuples, and optionals.
- RT-04 [todo] : Add round-trip tests for enums through canonical string tokens.
- RT-05 [todo] : Add round-trip tests for `one_of` with each supported alternative.
- RT-06 [todo] : Add round-trip tests that combine validators, enum mapping, and nested context.

## Phase 10: CI And Maintenance Coverage

- CI-01 [todo] : Add a dedicated native test target for runtime conformance.
- CI-02 [todo] : Add a separate compile-only target or script for negative template tests.
- CI-03 [todo] : Add coverage reporting or a simple coverage matrix document for supported spec nodes.
- CI-04 [todo] : Add a regression checklist requiring new spec nodes or options to ship with runtime and negative tests.
- CI-05 [todo] : Add a backlog follow-up for embedded-target smoke coverage once non-native environments are introduced.

## Definition Of Done

The spec-based serialization test backlog is complete when:

1. Every supported spec node has success, failure, and round-trip coverage where applicable.
2. Every supported option has positive and negative coverage.
3. Every supported runtime error code has at least one deterministic test.
4. Reflection fallback and reflection rejection rules are covered.
5. Compile-time invalid-use cases are enforced by automated checks, not manual review.
6. New features can be added by extending an established test matrix instead of inventing ad hoc coverage.