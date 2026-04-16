# Usage Guide

This document is the entry point for the shipped v1 usage surface of `lumalink-json`. The detailed reference has been split into focused documents so each topic can evolve without turning this page into a monolith.

## Guide Map

### Getting Started

Use [usage/01-getting-started-and-api.md](usage/01-getting-started-and-api.md) for:

- include and setup expectations
- the public deserialize, serialize, and schema entry points
- a minimal end-to-end example

### Spec Nodes

Use [usage/02-supported-spec-nodes.md](usage/02-supported-spec-nodes.md) for:

- the shipped spec-node surface
- C++ type mappings
- examples for scalar, string, enum, object, array, tuple, variant, and error bindings

### Options

Use [usage/03-supported-options.md](usage/03-supported-options.md) for:

- `opts::name`
- `opts::min_max_value`
- `opts::min_max_elements`
- `opts::pattern`
- `opts::not_empty`
- `opts::validator_func`
- built-in versus custom validation flow
- validation failure inspection patterns

### Binding And Runtime Behavior

Use [usage/04-object-binding-and-runtime-behavior.md](usage/04-object-binding-and-runtime-behavior.md) for:

- aggregate and explicit object binding modes
- error handling and context policies
- decode-time and encode-time validation flow
- raw JSON lifetime rules
- encoding into existing JSON storage

### Schema And Customization

Use [usage/05-schema-and-customization.md](usage/05-schema-and-customization.md) for:

- schema generation behavior
- custom decoder and encoder specializations
- explicit codec wrappers with `spec::with_codec`

### Constraints

Use [usage/06-constraints-and-gotchas.md](usage/06-constraints-and-gotchas.md) for the shipped v1 limits and caveats you need to account for when binding JSON to C++ models.

Use [usage/07-upgrading-to-v0.11.0.md](usage/07-upgrading-to-v0.11.0.md) for:

- a before-and-after migration example for explicit call-site object bindings
- prompt-ready upgrade instructions for downstream consumers still using `json::traits::object_fields`

## Recommended Reading Order

1. Start with [usage/01-getting-started-and-api.md](usage/01-getting-started-and-api.md).
2. Read [usage/02-supported-spec-nodes.md](usage/02-supported-spec-nodes.md) and [usage/03-supported-options.md](usage/03-supported-options.md) to define your wire contract.
3. Use [usage/04-object-binding-and-runtime-behavior.md](usage/04-object-binding-and-runtime-behavior.md) when binding real models and when tracing validation failures.
4. Use [usage/05-schema-and-customization.md](usage/05-schema-and-customization.md) when you need schema output or project-specific codecs.
5. Check [usage/06-constraints-and-gotchas.md](usage/06-constraints-and-gotchas.md) before treating an edge case as supported behavior.

## Where To Go Next

If you need implementation or design details beyond the usage set, the design documents start at [design/README.md](design/README.md).