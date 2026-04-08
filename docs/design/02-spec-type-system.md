# Spec Type System

## Design intent

The spec type system represents JSON shape as compile-time types. It is independent of runtime storage structs and drives both decode and encode behavior.

## Core spec nodes

1. scalar nodes
   - null
   - boolean
   - integer
   - number
   - string
   - enum_string
   - any
2. composition nodes
   - field<Key, ValueSpec>
   - object<Fields...>
   - array_of<ElementSpec>
   - tuple<Specs...>
   - optional<Spec>
   - one_of<Specs...>

## Object binding model

Each object field binds:

1. a JSON key
2. a child value spec
3. optional field-level options

The object spec is traversed in declaration order to support deterministic context and stable diagnostics.

For the automatic reflection path backed by pfr, target model types must be simple aggregates with stable field order.

When a type is not pfr-reflectable, the design uses explicit field-trait registration as fallback.

## Option system

Options are typed metadata attached to nodes:

1. name
2. validator_func
3. min_max_elements
4. min_max_value
5. pattern

For schema emission, `opts::pattern` may optionally carry a schema literal alongside the runtime predicate so the engine can project a JSON Schema `pattern` keyword.

Compile-time option parsing enforces valid combinations and rejects duplicates in the same category.

## C++ value mapping

Recommended defaults:

1. boolean -> bool
2. integer -> integral type selected by model field
3. number -> float or double selected by model field
4. string -> std::string_view or const char* based on model contract
5. enum_string<E> -> enum type E through declared string map
6. optional<Spec> -> std::optional<T>
7. one_of<...> -> std::variant<...>
8. tuple<...> -> std::tuple<...>
9. array_of<...> -> container with append semantics

## Enum mapping pattern

Canonical enum node:

1. spec::enum_string<E, Opts>
   - decode expects JSON string and resolves to enum E
   - encode expects enum E and emits mapped JSON string

Canonical mapping trait:

1. json::traits::enum_strings<E>
   - exposes a constexpr mapping table from string_view to E

Design rules:

1. mapping is bijective for encode safety
2. unknown input string returns an error, never a fallback enum value
3. duplicate string keys in the mapping are compile-time errors
4. validators can still run after successful enum resolution

## Validation contract

Validators are pure callables and must not throw.

Preferred signatures:

1. bool validator(const T&)
2. std::expected<void, json::error_code> validator(const T&)

The engine normalizes validator outcomes into std::expected<void, json::error> with context enrichment.
