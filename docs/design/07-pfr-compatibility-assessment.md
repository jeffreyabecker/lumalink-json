# PFR Compatibility Assessment

## Scope

This note validates the current design against the vendored pfr library in libs/pfr.

## Conclusion

The design is valid with pfr as the reflection backend, with explicit constraints.

Core compatibility is strong for index-based aggregate reflection used by object field traversal.

## Evidence summary

1. Reflection primitives are present
   - pfr::get<I>
   - pfr::tuple_size_v<T>
   - pfr::for_each_field
2. Aggregate enforcement is explicit
   - non-aggregate and polymorphic types are rejected by static assertions
   - inherited layouts are rejected by static assertions in field counting path
   - unions are forbidden in core reflection paths
3. Field-name reflection is optional and constrained
   - name extraction requires C++20+ support and parser compatibility
   - fallback implementation is intentionally disabled when requirements are unmet
4. Generated structured-binding support has a finite ceiling
   - current generated implementation supports field tags up to 200

## Design impact

1. Keep spec keys authoritative
   - JSON keys come from spec::field declarations, not pfr inferred names
2. Keep pfr usage index-based
   - bind object fields by compile-time order and index
3. Preserve fallback path
   - explicit field trait registration for types that are not pfr-reflectable
4. Enforce compile-time guardrails
   - static assert for unsupported model category
   - static assert for aggregate arity above 200 when using current generated backend

## Integration rules

1. Include policy
   - avoid umbrella pfr.hpp for embedded-focused builds
   - include only required core headers
2. Feature policy
   - do not require core_name APIs for baseline functionality
3. API policy
   - all decode and encode failures remain std::expected-based

## Recommended compile-time checks

1. reflectable category check
   - reject polymorphic, union, inherited aggregate, and non-aggregate model types on auto path
2. arity check
   - assert pfr::tuple_size_v<T> <= 200 for current vendor snapshot
3. fallback check
   - if auto path is unavailable, require explicit field trait registration

## Risk notes

1. Compiler-specific signature parsing can affect pfr field-name extraction.
2. Reflecting very large aggregates can hit generated arity limits.
3. Older toolchains may require configuration overrides in pfr config macros.

These risks are acceptable because baseline design does not depend on field-name reflection and already supports explicit traits as fallback.
