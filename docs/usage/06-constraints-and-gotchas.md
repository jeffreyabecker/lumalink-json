# Constraints And Gotchas

1. `spec::object` field count must match the bound model arity or explicit `object_fields` tuple arity.
2. `spec::object` auto-binding supports aggregates only; unions and polymorphic types require explicit trait registration and some shapes are still rejected.
3. The current PFR-backed auto-binding arity limit is 200 fields.
4. Optional object fields require `std::optional` members on the bound C++ type.
5. `spec::tuple` requires exact tuple length on decode.
6. `spec::one_of` requires `std::variant` order and arity to match the spec declaration exactly.
7. `spec::array_of` requires an append container; fixed-size arrays are not part of the shipped v1 surface.
8. `spec::any<>` is a pass-through JSON node, so the bound type is `JsonVariantConst`.
9. `spec::with_codec<InnerSpec, Codec>` requires `Codec::decode` and `Codec::encode` with the exact signatures shown above.
10. `Codec::emit_schema` is optional, but if you provide it the exact return type must be `json::expected_void`.
11. If you use `spec::with_codec<spec::any<>, Codec>` for `std::any`, the codec is responsible for all runtime type discrimination and for rejecting unsupported payloads safely.
12. `opts::validator_func` accepts only `bool`, `std::expected<void, json::error_code>`, or `json::expected_void` return types.
13. Runtime-only `opts::pattern<Predicate>` validation does not automatically imply a schema `pattern`; provide the schema literal explicitly if you need schema projection.
14. `std::string_view` and `const char*` decode targets depend on the lifetime of the backing `JsonDocument`.