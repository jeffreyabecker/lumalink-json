# Session Decomposition And Decisions

## Source artifact

- Session export analyzed: tue_apr_07_2026_designing_serialization_helper_with_validation.md

## Decomposed decision inventory

1. Build a spec-oriented JSON codec layer inspired by simple_asn1.
2. Use ArduinoJson as the underlying JSON DOM and parser backend.
3. Keep a dual-type model:
   - Spec types define structure and validation rules.
   - Plain C++ data types carry runtime values.
4. Preserve node-attached options and validators.
5. Preserve contextual error breadcrumbs for nested decode/encode failures.
6. Support extensibility through template specialization for custom types.
7. Operate without exceptions for embedded targets.
8. Standardize all fallible return values on std::expected.
9. Provide a canonical string-to-enum and enum-to-string mapping pattern.

## Corrections and normalizations

The session artifact contains references to a custom result<T> type. This is superseded by the project decision to use std::expected.

Canonical replacement pattern:

- Old pattern: result<T> with error member
- New pattern: std::expected<T, json::error>

- Old pattern: result<void>
- New pattern: std::expected<void, json::error>

## Scope decomposition into design docs

- Architecture baseline: 01-architecture-overview.md
- Spec model and mapping rules: 02-spec-type-system.md
- Error contract and context policy: 03-error-model-expected.md
- Traversal engine and extension hooks: 04-codec-engine-and-extensibility.md
- Toolchain constraints and rollout: 05-build-constraints-and-roadmap.md
- Enum mapping contract: 06-enum-string-mapping.md

## Naming note

The source export includes both lumawave-json and lumalink-json references. This repository uses lumalink-json as the working project name.
