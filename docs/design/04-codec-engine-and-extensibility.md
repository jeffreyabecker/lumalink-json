# Codec Engine And Extensibility

## Engine structure

The codec engine is a set of template specializations parameterized by:

1. Spec node type
2. C++ target value type
3. Traversal options
4. Parent context accumulator

For object-to-struct binding with pfr:

1. field count is resolved with pfr::tuple_size_v<T>
2. field access uses pfr::get<I>(value)
3. key names come from the spec field declarations, not inferred from struct member names

## Decoder contract

Each decoder specialization follows this shape:

std::expected<T, json::error> decode(JsonVariantConst src, const DecodeState& state);

Behavior:

1. check source JSON type compatibility
2. decode children recursively for composite nodes
3. run attached validators
4. return value on success or unexpected(error) on failure

## Encoder contract

Each encoder specialization follows this shape:

std::expected<void, json::error> encode(const T& value, JsonVariant dst, const EncodeState& state);

Behavior:

1. validate source value when required
2. materialize JSON values in destination variant
3. recurse for composite nodes
4. return unexpected(error) for any backend or policy violation

## Composite traversal rules

1. object
   - walk fields in declaration order
   - report missing required keys on decode
2. array_of
   - decode each element with index-aware context
   - apply element and container constraints
3. tuple
   - enforce exact position count unless relaxed by option
4. optional
   - treat null or missing as empty on decode
5. one_of
   - evaluate alternatives by strategy
   - default strategy is ordered attempt with first full success

## Enum string traversal rule

1. enum_string<E>
   - decode checks src is string, then resolves via enum mapping trait
   - encode resolves enum value back to canonical string token
   - both directions return std::expected with enum-specific error codes on failure

## Extensibility points

1. custom type decoders
   - specialize decoder for Spec and target type pair
2. custom type encoders
   - specialize encoder for Spec and source type pair
3. custom validators
   - attach via options
4. custom discrimination strategy for one_of
   - policy object in traversal options
5. custom enum mapping traits
   - specialize enum string table for project enum types

## Safety and determinism

1. no exceptions in control flow
2. all failure states represented by std::expected
3. no hidden heap usage in context propagation path
4. stable error context order for reproducible diagnostics
5. object binding does not depend on pfr field-name extraction APIs
