# JSON Schema Roadmap

Purpose: define and sequence the follow-on work required to grow `lumalink-json` from its current validation-oriented JSON Schema subset into a broader and more interoperable schema surface, without breaking the embedded-focused runtime model or the current spec-first API design.

Status legend:
- `todo`: not started
- `doing`: active
- `done`: completed
- `blocked`: waiting on dependency
- `deferred`: intentionally postponed

## Current Baseline

The shipped v1 schema emitter already covers a narrow but coherent subset of JSON Schema generation.

Current generated keywords and shapes:

- scalar `type`
- `object` `properties` and `required`
- `array_of` `items`, `minItems`, and `maxItems`
- `tuple` `prefixItems`, `minItems`, and `maxItems`
- `optional` as `anyOf` with value-or-null
- `one_of` as `anyOf`
- `enum_string` as string `enum`
- `min_max_value` as `minimum` and `maximum`
- `pattern` only when the option carries an explicit schema literal
- `not_empty` as `minLength: 1` for strings and `minItems: 1` for arrays
- additive metadata through `opts::schema<...>` contributors and codec `enrich_schema`

Current structural limitations that drive this roadmap:

- Full JSON Schema draft coverage is explicitly not a v1 goal.
- The current `spec::one_of` runtime behavior is ordered first-success matching, not exact `oneOf` semantics.
- The emitter has no reference or definition model today: no `$schema`, `$id`, `$defs`, or `$ref`.
- Object schemas do not express `additionalProperties`, `patternProperties`, `propertyNames`, `minProperties`, or `maxProperties`.
- Array schemas do not express `uniqueItems`, `contains`, `minContains`, `maxContains`, or `unevaluatedItems`.
- String and numeric constraints are incomplete relative to JSON Schema: no general `minLength`, `maxLength`, `multipleOf`, `exclusiveMinimum`, or `exclusiveMaximum`.
- Arbitrary runtime validators do not project into machine-checkable schema beyond the small built-in option set.

Backlog implication: the next work should extend the schema surface in layers, keeping the existing deterministic and allocation-aware design intact, and prioritizing interoperability for real downstream embedding and validation use cases rather than keyword completeness for its own sake.

## Guiding Principles

1. Keep the spec graph as the source of truth; do not introduce a parallel runtime schema DSL.
2. Prefer additive schema growth that composes with the current node model and option system.
3. Do not claim JSON Schema semantics the runtime model cannot defend or explain clearly.
4. Preserve deterministic schema output and embedded-friendly memory behavior.
5. Prefer embeddable schema fragments by default, because generated schemas are often included inside larger documents such as OpenAPI definitions.
6. Do not add new mappings solely to improve keyword coverage or draft completeness when they do not materially improve downstream use.
7. Add tests and documentation alongside each new keyword family so the supported surface stays explicit.

## Phase 0 Decisions

### JSR-00 - Canonical Vocabulary Baseline

Status: `todo`

Decide and document the canonical JSON Schema vocabulary baseline for generated output.

Recommended decision:

- target JSON Schema 2020-12 as the terminology and keyword baseline
- continue using `prefixItems` for tuples, which already aligns with 2020-12 style output
- do not emit document-level metadata such as `$schema` by default, because many generated schemas are embedded as fragments inside larger documents
- treat unsupported keywords as absent rather than emitting partial placeholders

Acceptance criteria:

- documentation states the intended vocabulary baseline explicitly
- roadmap items use dialect-correct terminology
- any future standalone-document `$schema` emission uses the chosen dialect URI consistently

### JSR-01 - Semantic Support Matrix

Status: `todo`

Add and maintain a machine-readable or documentation-readable matrix that distinguishes:

- runtime-supported semantics
- schema-emitted semantics
- annotation-only metadata
- explicitly unsupported schema features

Acceptance criteria:

- the matrix covers every public spec node and every schema-affecting option
- `one_of` is called out explicitly as not yet providing strict JSON Schema `oneOf` semantics
- the matrix becomes the update point for future schema work and docs

## Phase 1: Finish Core Scalar Constraints

### JSR-10 - General String Length Options And Schema Projection

Status: `todo`

Add declarative string-length options and emit `minLength` and `maxLength` for general string constraints, not only `not_empty`.

Acceptance criteria:

- specs can declare lower and upper string length bounds declaratively
- runtime decode and encode enforce the bounds
- schema generation emits `minLength` and `maxLength`
- `not_empty` composes predictably with the new option family

### JSR-11 - Exclusive Numeric Bounds

Status: `todo`

Add support for exclusive numeric lower and upper bounds and project them to `exclusiveMinimum` and `exclusiveMaximum`.

Acceptance criteria:

- the option surface expresses inclusive versus exclusive bounds without ambiguity
- runtime validation honors the declared semantics on decode and encode
- schema generation emits the correct exclusive bound keywords

### JSR-12 - `multipleOf` Projection And Runtime Enforcement

Status: `todo`

Add a numeric divisibility or step option that projects to `multipleOf` and enforces the same rule at runtime.

Acceptance criteria:

- integer and number nodes can declare step constraints
- runtime behavior is well-defined for integer and floating-point targets
- schema generation emits `multipleOf`
- documentation states any floating-point precision constraints explicitly

## Phase 2: Fill Core Object And Array Gaps

### JSR-20 - Object Property Count Constraints

Status: `todo`

Add object-level support for `minProperties` and `maxProperties` where the spec model can express a useful meaning.

Acceptance criteria:

- the option surface can attach property-count constraints to object nodes
- runtime decode validates incoming object property count
- schema generation emits `minProperties` and `maxProperties`
- behavior with optional fields is documented clearly

### JSR-21 - `additionalProperties` Policy

Status: `todo`

Introduce an explicit object policy for unknown keys and map it to `additionalProperties`.

Recommended first increment:

- boolean-only policy with default `true`
- when `true`, ignore additional properties during deserialization
- when `false`, reject additional properties during deserialization and return errors that identify the offending properties
- defer schema-valued `additionalProperties` until dynamic-key object support exists

Acceptance criteria:

- the option surface exposes a boolean policy with default `true`
- runtime decode ignores unexpected keys when the policy is `true`
- runtime decode rejects unexpected keys when the policy is `false`
- rejection errors include usable detail for the offending property names
- schema generation emits boolean `additionalProperties`
- the default policy is documented and covered by tests

### JSR-22 - Array Uniqueness

Status: `todo`

Add array uniqueness support and map it to `uniqueItems`.

Acceptance criteria:

- the option surface expresses uniqueness at the array node
- runtime decode and encode can validate uniqueness for supported element categories
- unsupported element comparison cases are rejected or documented explicitly
- schema generation emits `uniqueItems: true`

### JSR-23 - `contains` Family

Status: `todo`

Add `contains`, `minContains`, and `maxContains` support for arrays.

Design guardrail:

- this likely requires a new array-local child-spec concept distinct from homogeneous `items`
- do not overload `array_of<T>` in a way that makes runtime traversal ambiguous

Acceptance criteria:

- the spec system can express an array-level contains rule clearly
- runtime decode enforces the contains rule deterministically
- schema generation emits `contains`, `minContains`, and `maxContains` as applicable

## Phase 3: Correct Union And Composition Semantics

### JSR-30 - Separate Runtime `first_match` From Schema `oneOf`

Status: `todo`

Resolve the semantic mismatch between `spec::one_of` runtime behavior and emitted schema composition.

Viable paths:

- keep the current runtime type but rename or document it as ordered first-match and continue emitting `anyOf`
- add a distinct strict union surface that enforces exact single-match semantics and emits `oneOf`

Recommended direction:

- preserve existing `spec::one_of` behavior for compatibility
- add a new strict composition surface rather than silently changing semantics

Acceptance criteria:

- runtime and emitted schema semantics are aligned for each public union-like node
- ambiguity behavior is documented explicitly
- tests cover both ordered first-match and strict single-match variants if both surfaces exist

### JSR-31 - Add `allOf`

Status: `todo`

Add a composition node for conjunction-style schema and validation composition.

Acceptance criteria:

- the spec system can express combined constraints without custom codec replacement
- runtime decode composes all child validators deterministically
- schema generation emits `allOf`
- error reporting remains understandable when multiple child specs can fail

### JSR-32 - `const` And Single-Value Literals

Status: `todo`

Add a declarative constant-value surface that projects to `const`.

Acceptance criteria:

- specs can declare an exact required scalar or enum value
- runtime decode and encode enforce the value
- schema generation emits `const`

## Phase 4: Introduce Schema Reuse And References

### JSR-40 - Root Schema Metadata

Status: `todo`

Add optional support for standalone-document root metadata such as `$schema` and `$id` without making embedded-schema output awkward.

Acceptance criteria:

- the emitter can populate `$schema` and `$id` at the root when explicitly requested
- default generation behavior remains fragment-friendly for callers embedding schemas inside larger documents such as OpenAPI
- nested emitters do not add document-level metadata implicitly
- documentation states clearly that document-level metadata is opt-in and secondary to embeddable fragment generation

### JSR-41 - Definitions Model

Status: `todo`

Add a reusable definitions model using `$defs` and `$ref` so repeated or recursive shapes do not have to be emitted inline everywhere.

Acceptance criteria:

- the spec system has a deliberate way to name reusable definitions
- schema generation can emit `$defs`
- schema generation can reference reusable child schemas through `$ref`
- recursive shape support is explicitly either supported or deferred, not left implicit

### JSR-42 - Reference-Safe Customization Rules

Status: `todo`

Define how `opts::schema`, field-level contributors, and codec enrichment interact with referenced schemas.

Acceptance criteria:

- enrichment rules state whether annotations attach to the `$ref` site, the definition target, or both
- protected-key validation still behaves predictably in referenced schemas
- tests cover additive metadata on reused schema definitions

## Phase 5: Dynamic-Key Objects And Property Rules

### JSR-50 - `patternProperties`

Status: `todo`

Add first-class support for regex-keyed object members and map them to `patternProperties`.

Acceptance criteria:

- the spec system can express dynamic-key object members with a value spec
- runtime decode validates keys and values deterministically
- schema generation emits `patternProperties`
- interaction with fixed `properties` is documented clearly

### JSR-51 - `propertyNames`

Status: `todo`

Add key-schema validation for object member names and project it to `propertyNames`.

Acceptance criteria:

- runtime decode can validate object keys against a declared rule
- schema generation emits `propertyNames`
- the feature composes with `patternProperties` and `additionalProperties`

### JSR-52 - Schema-Valued `additionalProperties`

Status: `todo`

Extend the `additionalProperties` policy from boolean-only to schema-valued extra members when dynamic-key object support exists.

Acceptance criteria:

- specs can declare a value spec for extra properties
- runtime decode validates unknown-key values through the extra-property spec
- schema generation emits schema-valued `additionalProperties`

## Phase 6: Dependency And Conditional Features

### JSR-60 - `dependentRequired`

Status: `todo`

Add conditional required-key relationships between object fields.

Acceptance criteria:

- the spec system can express field dependencies declaratively
- runtime decode reports missing dependent fields with usable context
- schema generation emits `dependentRequired`

### JSR-61 - `dependentSchemas`

Status: `todo`

Add object-local dependent schema application.

Acceptance criteria:

- specs can declare schema fragments that activate when a key is present
- runtime validation and schema emission remain aligned
- the error surface stays understandable for nested dependency failures

### JSR-62 - `if` / `then` / `else`

Status: `todo`

Add conditional composition for schemas that depend on discriminator-like structure without requiring custom codecs.

Acceptance criteria:

- the spec system can express conditional schema logic without ad hoc validator code
- runtime behavior is deterministic and documented
- schema generation emits `if`, `then`, and `else`

## Phase 7: Documentation, Tests, And Exit Criteria

### JSR-70 - Schema Conformance Test Expansion

Status: `todo`

Extend native tests and schema snapshot coverage for every newly supported keyword family.

Acceptance criteria:

- every roadmap item that lands ships with runtime and schema-generation tests where applicable
- negative tests cover unsupported or conflicting option combinations
- schema output stability is tested where order matters

### JSR-71 - Usage Guide Update

Status: `todo`

Update the usage docs so the published schema surface stays explicit and non-aspirational.

Acceptance criteria:

- the supported keyword list is updated in the schema usage docs
- constraints and gotchas call out semantics that differ from full JSON Schema expectations
- roadmap-delivered features are documented in the same release window as the code

### JSR-72 - Coverage Creep Review

Status: `deferred`

Re-evaluate whether any remaining unsupported keywords are worth pursuing after the higher-value interoperability features above land.

Review criteria:

- count remaining unsupported keywords after phases 1 through 6
- evaluate code size and complexity growth on embedded targets
- evaluate whether the missing features still block real downstream use cases
- reject additions whose only justification is schema completeness rather than practical downstream value

## Recommended Delivery Order

Recommended implementation order, biased toward highest value with the lowest architectural risk:

1. JSR-00 and JSR-01
2. JSR-10 through JSR-12
3. JSR-21 and JSR-20
4. JSR-30 and JSR-32
5. JSR-22
6. JSR-41 through JSR-42
7. JSR-50 through JSR-52
8. JSR-60 through JSR-62
9. JSR-31 and JSR-23 where real caller demand justifies the extra complexity
10. JSR-40 only if a real standalone-document publishing need appears

Rationale:

- scalar constraints and unknown-key policy are straightforward extensions of the current option-driven design
- strict union semantics need an explicit design choice before more composition nodes are added
- `$defs` and `$ref` should come before large dynamic-key and conditional schema surfaces so reuse semantics are not retrofitted later
- `contains`, `allOf`, and conditional composition are valuable, but they introduce more design branching than the earlier items
- document-level `$schema` and `$id` are lower priority because many consumers embed these schemas inside larger documents rather than publishing them as standalone schema files

## Definition Of Done

This roadmap is materially complete when:

1. The supported schema dialect and semantic support matrix are explicit and maintained.
2. The library can express common scalar, object, array, and union constraints without relying on opaque custom validators for routine cases.
3. Runtime behavior and emitted schema semantics are aligned for each public composition construct.
4. Reusable schema definitions and references exist for repeated or recursive shapes.
5. Dynamic-key object modeling is possible without dropping to full custom codec replacement.
6. Documentation and tests describe the actual supported schema surface precisely enough that downstream users do not have to infer it from implementation details.
7. The repo can make an informed, evidence-based decision on whether any remaining additions have practical downstream value rather than merely increasing keyword coverage.