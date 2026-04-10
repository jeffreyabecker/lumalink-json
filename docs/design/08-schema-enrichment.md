# Schema Enrichment

## Purpose

Define a post-v1 extension that lets specs and codecs add richer schema metadata without replacing the base generated schema shape.

The current schema surface covers structural keywords such as `type`, `properties`, `required`, `items`, `enum`, and a narrow set of option-driven projections. That is enough for validation-oriented schemas, but not for documentation-oriented or integration-oriented schema publishing.

## Problem Statement

Today there are only two schema customization levels:

1. built-in projection from a small fixed option set
2. full replacement through `Codec::emit_schema`

This leaves a gap for common cases:

1. attaching `title`, `description`, or `format` to a node
2. exposing examples or defaults for generated documentation
3. projecting field-level documentation without duplicating the whole child schema
4. extending wrapped specs in a codec without re-implementing the inner schema emission logic

The all-or-nothing codec override is too blunt for those cases. It increases duplication and creates drift risk when the wrapped spec gains new built-in schema behavior later.

## Goals

1. Preserve the existing generated schema as the canonical structural baseline.
2. Allow additive schema metadata on any node that already accepts options.
3. Allow object fields to carry documentation-oriented schema metadata.
4. Allow codecs to enrich the generated schema after the inner spec has emitted its baseline shape.
5. Keep decode and encode behavior unchanged.
6. Keep schema generation deterministic and allocation-aware for embedded targets.

## Non-Goals

1. Full JSON Schema draft coverage.
2. Runtime schema authoring or runtime option registration.
3. Implicit projection of arbitrary validator semantics into machine-checkable schema.
4. Replacing the existing `Codec::emit_schema` full-override hook.

## Design Summary

Schema enrichment is an additive layer that runs after the base schema for a node has been emitted.

The design has two authoring surfaces:

1. spec-level schema contribution options
2. codec-level schema enrichment hooks

The base emitter remains responsible for structural keywords. Enrichment only adds or amends allowed metadata keywords on the already-generated node.

## Authoring Model

### 1. Spec-Level Enrichment

Introduce a repeatable option category:

`opts::schema<Contributor>`

Contributor contract:

```cpp
struct contributor {
    static json::expected_void apply(JsonVariant schema);
};
```

Behavior:

1. the contributor receives the node-local emitted schema slot
2. it may add or update enrichment keywords
3. it must not erase required structural keywords emitted by the base node
4. it returns `json::expected_void`

This provides a single extensible hook instead of adding a separate option family for every possible schema keyword.

### 2. Built-In Contributors

The library should ship a small built-in contributor set for common documentation keywords.

Recommended first set:

1. `schema_meta::title<"...">`
2. `schema_meta::description<"...">`
3. `schema_meta::format<"...">`
4. `schema_meta::deprecated<true>`
5. `schema_meta::read_only<true>`
6. `schema_meta::write_only<true>`

These are intentionally annotation-oriented. They do not change runtime validation behavior.

### 3. Literal JSON Payload Contributors

Some schema keywords need richer payloads than a single string or boolean, for example:

1. `examples`
2. `default`
3. vendor extension objects such as `x-*`

For those cases, the built-in surface should allow a contributor type to write arbitrary JSON into the schema slot instead of trying to encode every possible keyword shape into template parameters.

This keeps the core API small while still covering advanced publisher needs.

### 4. Field-Level Enrichment

`spec::field<Key, ValueSpec, Options...>` should accept `opts::schema<...>` options and apply them to the generated property schema for that field.

This is important because many documentation-oriented annotations belong to the wire key, not to the reusable value spec alone.

Example intent:

```cpp
using endpoint_spec = json::spec::object<
    json::spec::field<
        "token",
        json::spec::string<>,
        json::opts::schema<json::schema_meta::description<"API token shown in setup UI">>>;
```

## Codec Model

Keep the existing full override hook:

```cpp
template <typename InnerSpec>
static json::expected_void emit_schema(JsonVariant destination);
```

Add a preferred additive hook:

```cpp
template <typename InnerSpec>
static json::expected_void enrich_schema(JsonVariant destination);
```

Hook semantics:

1. `emit_schema` remains full replacement
2. `enrich_schema` runs after `InnerSpec` has emitted its baseline schema
3. if both hooks exist, `emit_schema` wins and `enrich_schema` is skipped

This gives codecs a way to annotate wrapped schemas without copying the entire inner schema shape.

## Emission Order

For any node, schema generation should run in this order:

1. emit built-in structural keywords for the node type
2. project existing built-in option-derived keywords such as `minimum`, `maximum`, `minItems`, `maxItems`, and `pattern`
3. apply spec-level `opts::schema<...>` contributors in declaration order
4. if the node is reached through `spec::field`, apply field-level contributors in declaration order
5. if the node is wrapped in `spec::with_codec` and the codec provides `enrich_schema`, apply the codec hook last

Rationale:

1. structural generation stays canonical
2. user-supplied enrichment can build on already-emitted shape
3. field-level annotations can refine property-local documentation
4. codec enrichment gets the final view of the wrapped node

## Conflict Rules

Schema enrichment needs deterministic overwrite policy.

Recommended rules:

1. built-in structural emission owns required structural keywords
2. additive contributors may append new keys freely
3. additive contributors may overwrite annotation keywords such as `title`, `description`, `format`, `deprecated`, `readOnly`, and `writeOnly`
4. additive contributors must not overwrite structural keys such as `type`, `properties`, `items`, `required`, `prefixItems`, or `enum`
5. attempts to overwrite protected structural keys should fail with `error_code::validation_failed` and a clear schema-generation message

This keeps the enrichment layer additive in practice instead of becoming a second full schema emitter.

## Option-System Impact

The option parser needs one new repeatable option category:

1. `schema contributor`

Rules:

1. repeatable by design
2. valid on scalar nodes, `array_of`, and `field`
3. not accepted on `object`, `tuple`, `optional`, or `one_of` in the first increment unless a concrete need appears

The first increment should avoid broadening every node category unless there is a clear publishing use case.

## Error Handling

Schema enrichment remains a fallible operation and keeps the current `json::expected_void` contract.

Representative failure cases:

1. contributor writes an invalid payload for the active destination type
2. contributor attempts to replace a protected structural keyword
3. contributor fails to materialize a nested JSON object or array because the backend has no memory

These failures should return the same `json::error` surface already used by schema generation.

## Backward Compatibility

1. existing specs continue to emit the same schemas
2. existing `Codec::emit_schema` overrides continue to work unchanged
3. schema enrichment is purely additive and opt-in
4. no decode or encode API signatures change

## Testing Requirements

1. contributor-applied `title`, `description`, and `format` on scalar nodes
2. field-level description on object properties
3. multiple contributors on the same node preserve declaration order and final values deterministically
4. codec `enrich_schema` augments the inner schema instead of replacing it
5. `emit_schema` still fully replaces the inner schema and suppresses `enrich_schema`
6. protected structural key overwrite attempts fail predictably
7. no schema regression for existing non-enriched specs

## Incremental Rollout

1. add contributor option plumbing and built-in annotation contributors
2. apply enrichment support to scalar nodes, `array_of`, and `field`
3. add codec `enrich_schema` hook support
4. add tests that cover additive and override interactions
5. document the preferred authoring path: enrichment first, full codec override only when shape replacement is truly required

## Open Questions

1. Should `spec::object` itself accept contributors for object-level descriptions in the first increment, or should that wait until there is a concrete caller need?
2. Should `examples` and `default` ship as first-party helpers, or should they stay in user-defined contributors until the literal-authoring ergonomics settle?
3. Should protected structural keys be rejected uniformly, or should some keys such as `required` remain extendable through dedicated helpers later?