# Design Documents

This folder decomposes the session export into focused design documents for the lumalink-json project.

## Document map

1. [00-decomposition-and-decisions.md](00-decomposition-and-decisions.md)
   - Extracted decisions from the session export
   - Corrected architectural baselines
   - Canonical constraints for follow-on implementation

2. [01-architecture-overview.md](01-architecture-overview.md)
   - Scope, goals, non-goals
   - Layered architecture and primary APIs

3. [02-spec-type-system.md](02-spec-type-system.md)
   - Spec tree model and option system
   - Mapping from JSON shape to C++ types

4. [03-error-model-expected.md](03-error-model-expected.md)
   - Error taxonomy and context model
   - Return-type contract using std::expected

5. [04-codec-engine-and-extensibility.md](04-codec-engine-and-extensibility.md)
   - Decoder/encoder traversal model
   - Custom converter and validator extension points

6. [05-build-constraints-and-roadmap.md](05-build-constraints-and-roadmap.md)
   - Toolchain expectations
   - Phased implementation plan and open questions

7. [06-enum-string-mapping.md](06-enum-string-mapping.md)
   - Canonical pattern for string <-> enum conversion
   - Traits, error behavior, and examples

8. [07-pfr-compatibility-assessment.md](07-pfr-compatibility-assessment.md)
   - Validation of design assumptions against libs/pfr
   - Required constraints and integration rules

9. [08-schema-enrichment.md](08-schema-enrichment.md)
   - Additive schema metadata model for specs and codecs
   - Conflict rules, ordering, and rollout guidance

## Canonical policy from this design set

All fallible APIs return std::expected.
No custom result wrapper class is introduced.
