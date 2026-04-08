# Architecture Overview

## Purpose

lumalink-json defines a compile-time spec system for structured JSON validation and conversion over ArduinoJson. The design is inspired by simple_asn1, adapted for JSON and embedded constraints.

## Goals

1. Declarative spec types in C++ templates.
2. Direct C++ model <-> JSON document conversion.
3. No exception dependency in library behavior.
4. Validation as part of traversal, not a separate pass.
5. Deterministic and low-overhead behavior on embedded targets.
6. API-level error signaling through std::expected.
7. First-class mapping for JSON strings to strongly typed enums.

## Non-goals

1. JSON Schema draft compatibility in v1.
2. Runtime schema compilation.
3. Reflection dependency on large external libraries.
4. Hidden dynamic allocation in core traversal logic.

## Layered model

1. Public API layer
   - deserialize and serialize entry points
   - schema generation entry point
   - decode and encode options
2. Spec layer
   - primitive and composite spec nodes
   - field binding and optionality semantics
3. Engine layer
   - decoder and encoder template specializations
   - node-by-node traversal and coercion checks
4. Error layer
   - error_code and error context stack
   - std::expected propagation
5. Backend adapter layer
   - ArduinoJson JsonVariant and JsonDocument interaction

## Primary API shape

- deserialize:
  std::expected<T, json::error> deserialize<T, Spec>(JsonVariantConst src);
   std::expected<T, json::error> deserialize<T, Spec>(std::string_view json);

- serialize:
  std::expected<void, json::error> serialize<Spec>(const T& value, JsonVariant dst);

- generate_schema:
   std::expected<void, json::error> generate_schema<Spec>(JsonDocument& dst);

## Behavioral principles

1. Structural mismatch is a typed error, never undefined behavior.
2. Validation failures return error with path context.
3. Nested failures preserve parent context based on policy.
4. All node handlers are composable through expected-return chaining.
