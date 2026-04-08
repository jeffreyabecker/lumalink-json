#pragma once

#include <ArduinoJson.h>

#include <string>
#include <tuple>
#include <type_traits>

#include <json/core.h>
#include <json/options.h>
#include <json/spec.h>
#include <json/traits.h>

namespace lumalink::json::detail {

template <typename Spec>
struct is_schema_optional_spec : std::false_type {};

template <typename ValueSpec>
struct is_schema_optional_spec<spec::optional<ValueSpec>> : std::true_type {};

template <typename Spec>
inline constexpr bool is_schema_optional_spec_v = is_schema_optional_spec<Spec>::value;

template <typename Enum, typename = void>
struct has_schema_enum_strings : std::false_type {};

template <typename Enum>
struct has_schema_enum_strings<Enum, std::void_t<decltype(traits::enum_strings<Enum>::values)>> : std::true_type {};

template <typename Spec>
void emit_min_max_value_keywords(JsonObject schema) {
    using range_option = typename spec_descriptor<Spec>::min_max_value_option;

    if constexpr (!std::is_void_v<range_option>) {
        schema["minimum"] = range_option::min;
        schema["maximum"] = range_option::max;
    }
}

template <typename Spec>
void emit_min_max_elements_keywords(JsonObject schema) {
    using range_option = typename spec_descriptor<Spec>::min_max_elements_option;

    if constexpr (!std::is_void_v<range_option>) {
        schema["minItems"] = range_option::min;
        schema["maxItems"] = range_option::max;
    }
}

template <typename Spec>
void emit_pattern_keyword(JsonObject schema) {
    using pattern_option = typename spec_descriptor<Spec>::pattern_option;

    if constexpr (!std::is_void_v<pattern_option>) {
        if constexpr (!pattern_option::schema().empty()) {
            schema["pattern"] = std::string(pattern_option::schema());
        }
    }
}

template <typename Spec>
struct schema_emitter {
    static expected_void emit(JsonVariant) {
        static_assert(always_false_v<Spec>, "schema generation is not implemented for this spec node");
        return std::unexpected(error{error_code::not_implemented, {}, "schema generation is not implemented"});
    }
};

template <typename FieldSpec>
inline constexpr bool is_required_field_v = !is_schema_optional_spec_v<typename FieldSpec::value_spec>;

template <typename... Fields>
inline constexpr size_t required_field_count_v =
    (0U + ... + static_cast<size_t>(is_required_field_v<Fields>));

template <typename FieldSpec>
expected_void emit_object_field_schema(JsonObject properties, JsonArray required) {
    JsonVariant property_schema = properties[FieldSpec::key_c_str()].template to<JsonVariant>();
    auto result = schema_emitter<typename FieldSpec::value_spec>::emit(property_schema);
    if (!result.has_value()) {
        return result;
    }

    if constexpr (is_required_field_v<FieldSpec>) {
        required.add(FieldSpec::key_c_str());
    }

    return {};
}

template <typename... Fields>
struct object_field_schema_emitter;

template <>
struct object_field_schema_emitter<> {
    static expected_void emit(JsonObject, JsonArray) {
        return {};
    }
};

template <typename FirstField, typename... Rest>
struct object_field_schema_emitter<FirstField, Rest...> {
    static expected_void emit(JsonObject properties, JsonArray required) {
        auto result = emit_object_field_schema<FirstField>(properties, required);
        if (!result.has_value()) {
            return result;
        }

        return object_field_schema_emitter<Rest...>::emit(properties, required);
    }
};

template <typename... Specs>
struct schema_array_emitter;

template <>
struct schema_array_emitter<> {
    static expected_void emit(JsonArray) {
        return {};
    }
};

template <typename FirstSpec, typename... Rest>
struct schema_array_emitter<FirstSpec, Rest...> {
    static expected_void emit(JsonArray destination) {
        JsonVariant child_schema = destination.add<JsonVariant>();
        auto result = schema_emitter<FirstSpec>::emit(child_schema);
        if (!result.has_value()) {
            return result;
        }

        return schema_array_emitter<Rest...>::emit(destination);
    }
};

template <typename... Options>
struct schema_emitter<spec::null<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "null";
        return {};
    }
};

template <typename... Options>
struct schema_emitter<spec::boolean<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "boolean";
        return {};
    }
};

template <typename... Options>
struct schema_emitter<spec::integer<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "integer";
        emit_min_max_value_keywords<spec::integer<Options...>>(schema);
        return {};
    }
};

template <typename... Options>
struct schema_emitter<spec::number<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "number";
        emit_min_max_value_keywords<spec::number<Options...>>(schema);
        return {};
    }
};

template <typename... Options>
struct schema_emitter<spec::string<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "string";
        emit_pattern_keyword<spec::string<Options...>>(schema);
        return {};
    }
};

template <typename Enum, typename... Options>
struct schema_emitter<spec::enum_string<Enum, Options...>> {
    static expected_void emit(JsonVariant destination) {
        static_assert(
            has_schema_enum_strings<Enum>::value,
            "enum_string requires json::traits::enum_strings specialization for schema generation");

        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "string";

        JsonArray enum_values = schema["enum"].template to<JsonArray>();
        for (const auto& entry : traits::enum_strings<Enum>::values) {
            enum_values.add(std::string(entry.token));
        }

        return {};
    }
};

template <typename... Options>
struct schema_emitter<spec::any<Options...>> {
    static expected_void emit(JsonVariant destination) {
        destination.to<JsonObject>();
        return {};
    }
};

template <fixed_string Key, typename ValueSpec, typename... Options>
struct schema_emitter<spec::field<Key, ValueSpec, Options...>> {
    static expected_void emit(JsonVariant destination) {
        return schema_emitter<ValueSpec>::emit(destination);
    }
};

template <typename... Fields>
struct schema_emitter<spec::object<Fields...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "object";

        JsonObject properties = schema["properties"].template to<JsonObject>();
        JsonArray required;
        if constexpr (required_field_count_v<Fields...> > 0U) {
            required = schema["required"].template to<JsonArray>();
        }

        return object_field_schema_emitter<Fields...>::emit(properties, required);
    }
};

template <typename ElementSpec, typename... Options>
struct schema_emitter<spec::array_of<ElementSpec, Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "array";
        emit_min_max_elements_keywords<spec::array_of<ElementSpec, Options...>>(schema);

        JsonVariant item_schema = schema["items"].template to<JsonVariant>();
        return schema_emitter<ElementSpec>::emit(item_schema);
    }
};

template <typename... Specs>
struct schema_emitter<spec::tuple<Specs...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "array";
        schema["minItems"] = sizeof...(Specs);
        schema["maxItems"] = sizeof...(Specs);

        JsonArray prefix_items = schema["prefixItems"].template to<JsonArray>();
        return schema_array_emitter<Specs...>::emit(prefix_items);
    }
};

template <typename ValueSpec>
struct schema_emitter<spec::optional<ValueSpec>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        JsonArray any_of = schema["anyOf"].template to<JsonArray>();

        JsonVariant value_schema = any_of.add<JsonVariant>();
        auto value_result = schema_emitter<ValueSpec>::emit(value_schema);
        if (!value_result.has_value()) {
            return value_result;
        }

        JsonVariant null_schema = any_of.add<JsonVariant>();
        return schema_emitter<spec::null<>>::emit(null_schema);
    }
};

template <typename... Specs>
struct schema_emitter<spec::one_of<Specs...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        JsonArray any_of = schema["anyOf"].template to<JsonArray>();
        return schema_array_emitter<Specs...>::emit(any_of);
    }
};

} // namespace lumalink::json::detail

namespace lumalink::json {

template <typename Spec>
expected_void generate_schema(JsonVariant destination) {
    return detail::schema_emitter<Spec>::emit(destination);
}

template <typename Spec>
expected_void generate_schema(JsonDocument& document) {
    document.clear();
    JsonVariant destination = document.to<JsonVariant>();
    return generate_schema<Spec>(destination);
}

} // namespace lumalink::json