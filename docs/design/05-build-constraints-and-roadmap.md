# Build Constraints And Roadmap

## Toolchain requirements

1. language level
   - C++23 required for standard std::expected
2. exceptions
   - library logic must not require exceptions
   - usable with exception-disabled builds
3. backend
   - ArduinoJson 7.x as primary JSON backend
4. dependencies
   - avoid mandatory heavy reflection dependencies
5. reflection backend option
   - vendored libs/pfr is an acceptable backend for aggregate field access

## Compatibility strategy

1. baseline profile
   - C++23 + ArduinoJson 7.x
2. optional profile
   - if some embedded toolchains lag on C++23, the project can gate build support by compiler version and standard library feature test macros
3. explicit non-goal
   - no custom result wrapper introduced to emulate expected semantics

## PFR integration constraints

Validated against libs/pfr in this repository:

1. supported model types
   - complete, non-reference, non-polymorphic aggregate types
   - no unions in reflection path
   - no inherited aggregate base layout in reflection path
2. generated structured-binding arity
   - current generated core supports up to 200 fields
   - add a compile-time guard for object specs that target larger aggregates
3. field name extraction
   - pfr field-name APIs require C++20+ and parser compatibility
   - design must not require inferred field names for core operation
4. include policy for embedded builds
   - avoid umbrella include pfr.hpp
   - include only required headers for core reflection primitives

## Incremental implementation plan

1. phase 1: core types
   - error_code
   - error context representation
   - public expected aliases
2. phase 2: scalar decode and encode
   - null, boolean, integer, number, string, enum_string
3. phase 3: composite nodes
   - object, field, array_of, optional, tuple
4. phase 4: one_of and policy hooks
   - alternative selection and diagnostics
5. phase 5: extension API hardening
   - custom converter docs and conformance tests
6. phase 6: optimization pass
   - context policy tuning
   - code-size and stack-depth review

## Testing roadmap

1. golden success cases per node type
2. structured failure cases with expected context path
3. validator behavior matrix
4. backend parse error mapping matrix
5. compile-time option parser negative tests
6. enum string mapping tests
   - unknown token decode failure
   - unmapped enum encode failure
   - duplicate mapping compile-time rejection

## Open questions carried from source session

1. reflection strategy
   - minimal pfr-like support vs explicit field trait registration
2. one_of strategy
   - ordered attempts vs discriminator field
3. string representation
   - const char* and string_view semantics under ArduinoJson lifetime rules
4. context depth budget
   - default depth for constrained microcontrollers

Resolved direction:

1. use vendored pfr for aggregate index-based reflection
2. keep explicit field-trait fallback for unsupported model types
