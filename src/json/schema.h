#pragma once

#include <ArduinoJson.h>

#include <algorithm>
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

template <typename EnumOrCodec, typename = void>
struct has_schema_enum_mapping : std::false_type {};

template <typename EnumOrCodec>
struct has_schema_enum_mapping<
    EnumOrCodec,
    std::void_t<decltype(detail::enum_mapping_provider<EnumOrCodec>::values())>> : std::true_type {};

template <typename Codec, typename InnerSpec, typename = void>
struct has_wrapped_codec_schema : std::false_type {};

template <typename Codec, typename InnerSpec>
struct has_wrapped_codec_schema<
    Codec,
    InnerSpec,
    std::void_t<decltype(Codec::template emit_schema<InnerSpec>(std::declval<JsonVariant>()))>> : std::true_type {};

template <typename Codec, typename InnerSpec>
inline constexpr bool has_wrapped_codec_schema_v = has_wrapped_codec_schema<Codec, InnerSpec>::value;

template <typename Codec, typename InnerSpec, typename = void>
struct has_wrapped_codec_schema_enrichment : std::false_type {};

template <typename Codec, typename InnerSpec>
struct has_wrapped_codec_schema_enrichment<
    Codec,
    InnerSpec,
    std::void_t<decltype(Codec::template enrich_schema<InnerSpec>(std::declval<JsonVariant>()))>> : std::true_type {};

template <typename Codec, typename InnerSpec>
inline constexpr bool has_wrapped_codec_schema_enrichment_v = has_wrapped_codec_schema_enrichment<Codec, InnerSpec>::value;

template <typename Spec>
struct schema_emitter;

template <typename Spec>
struct base_schema_emitter;

template <typename Spec>
struct protected_schema_keyword_validator {
    static expected_void validate(JsonVariantConst, JsonVariantConst) {
        return {};
    }
};

template <typename Spec>
struct spec_schema_contributors {
    using type = type_list<>;
};

template <typename... Options>
struct spec_schema_contributors<spec::null<Options...>> {
    using type = schema_contributor_list_t<Options...>;
};

template <typename... Options>
struct spec_schema_contributors<spec::boolean<Options...>> {
    using type = schema_contributor_list_t<Options...>;
};

template <typename... Options>
struct spec_schema_contributors<spec::integer<Options...>> {
    using type = schema_contributor_list_t<Options...>;
};

template <typename... Options>
struct spec_schema_contributors<spec::number<Options...>> {
    using type = schema_contributor_list_t<Options...>;
};

template <typename... Options>
struct spec_schema_contributors<spec::string<Options...>> {
    using type = schema_contributor_list_t<Options...>;
};

template <typename EnumOrCodec, typename... Options>
struct spec_schema_contributors<spec::enum_string<EnumOrCodec, Options...>> {
    using type = schema_contributor_list_t<Options...>;
};

template <typename... Options>
struct spec_schema_contributors<spec::any<Options...>> {
    using type = schema_contributor_list_t<Options...>;
};

template <fixed_string Key, typename ValueSpec, typename... Options>
struct spec_schema_contributors<spec::field<Key, ValueSpec, Options...>> {
    using type = schema_contributor_list_t<Options...>;
};

template <typename ElementSpec, typename... Options>
struct spec_schema_contributors<spec::array_of<ElementSpec, Options...>> {
    using type = schema_contributor_list_t<Options...>;
};

inline expected_void protected_schema_keyword_error() {
    return std::unexpected(error{
        error_code::validation_failed,
        {},
        "schema enrichment cannot overwrite protected structural keywords"});
}

inline bool json_variants_equal(JsonVariantConst left, JsonVariantConst right) {
    if (left.isUnbound() || right.isUnbound()) {
        return left.isUnbound() == right.isUnbound();
    }

    std::string left_serialized;
    std::string right_serialized;
    serializeJson(left, left_serialized);
    serializeJson(right, right_serialized);
    return left_serialized == right_serialized;
}

inline expected_void validate_object_keyword_matches(
    const JsonVariantConst actual_schema,
    const JsonVariantConst baseline_schema,
    const char* key) {
    if (!actual_schema.is<JsonObjectConst>() || !baseline_schema.is<JsonObjectConst>()) {
        return protected_schema_keyword_error();
    }

    JsonObjectConst actual_object = actual_schema.as<JsonObjectConst>();
    JsonObjectConst baseline_object = baseline_schema.as<JsonObjectConst>();

    JsonVariantConst actual_value = actual_object[key];
    JsonVariantConst baseline_value = baseline_object[key];
    const bool actual_has_value = !actual_value.isUnbound();
    const bool baseline_has_value = !baseline_value.isUnbound();

    if (actual_has_value != baseline_has_value) {
        return protected_schema_keyword_error();
    }

    if (!baseline_has_value) {
        return {};
    }

    if (!json_variants_equal(actual_value, baseline_value)) {
        return protected_schema_keyword_error();
    }

    return {};
}

template <typename Spec>
expected_void validate_protected_object_keywords(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
    if (auto type_result = validate_object_keyword_matches(actual_schema, baseline_schema, "type"); !type_result.has_value()) {
        return type_result;
    }

    return {};
}

template <typename... Options>
struct protected_schema_keyword_validator<spec::null<Options...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        return validate_protected_object_keywords<spec::null<Options...>>(actual_schema, baseline_schema);
    }
};

template <typename... Options>
struct protected_schema_keyword_validator<spec::boolean<Options...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        return validate_protected_object_keywords<spec::boolean<Options...>>(actual_schema, baseline_schema);
    }
};

template <typename... Options>
struct protected_schema_keyword_validator<spec::integer<Options...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        return validate_protected_object_keywords<spec::integer<Options...>>(actual_schema, baseline_schema);
    }
};

template <typename... Options>
struct protected_schema_keyword_validator<spec::number<Options...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        return validate_protected_object_keywords<spec::number<Options...>>(actual_schema, baseline_schema);
    }
};

template <typename... Options>
struct protected_schema_keyword_validator<spec::string<Options...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        return validate_protected_object_keywords<spec::string<Options...>>(actual_schema, baseline_schema);
    }
};

template <typename EnumOrCodec, typename... Options>
struct protected_schema_keyword_validator<spec::enum_string<EnumOrCodec, Options...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        auto type_result = validate_protected_object_keywords<spec::enum_string<EnumOrCodec, Options...>>(
            actual_schema,
            baseline_schema);
        if (!type_result.has_value()) {
            return type_result;
        }

        return validate_object_keyword_matches(actual_schema, baseline_schema, "enum");
    }
};

template <typename... Options>
struct protected_schema_keyword_validator<spec::any<Options...>> {
    static expected_void validate(JsonVariantConst, JsonVariantConst) {
        return {};
    }
};

template <fixed_string Key, typename ValueSpec, typename... Options>
struct protected_schema_keyword_validator<spec::field<Key, ValueSpec, Options...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        return protected_schema_keyword_validator<ValueSpec>::validate(actual_schema, baseline_schema);
    }
};

template <typename... Fields>
struct protected_schema_keyword_validator<spec::object<Fields...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        auto type_result = validate_protected_object_keywords<spec::object<Fields...>>(actual_schema, baseline_schema);
        if (!type_result.has_value()) {
            return type_result;
        }

        if (auto properties_result = validate_object_keyword_matches(actual_schema, baseline_schema, "properties");
            !properties_result.has_value()) {
            return properties_result;
        }

        return validate_object_keyword_matches(actual_schema, baseline_schema, "required");
    }
};

template <typename ElementSpec, typename... Options>
struct protected_schema_keyword_validator<spec::array_of<ElementSpec, Options...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        auto type_result = validate_protected_object_keywords<spec::array_of<ElementSpec, Options...>>(
            actual_schema,
            baseline_schema);
        if (!type_result.has_value()) {
            return type_result;
        }

        return validate_object_keyword_matches(actual_schema, baseline_schema, "items");
    }
};

template <typename... Specs>
struct protected_schema_keyword_validator<spec::tuple<Specs...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        auto type_result = validate_protected_object_keywords<spec::tuple<Specs...>>(actual_schema, baseline_schema);
        if (!type_result.has_value()) {
            return type_result;
        }

        return validate_object_keyword_matches(actual_schema, baseline_schema, "prefixItems");
    }
};

template <typename ValueSpec>
struct protected_schema_keyword_validator<spec::optional<ValueSpec>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        return validate_object_keyword_matches(actual_schema, baseline_schema, "anyOf");
    }
};

template <>
struct protected_schema_keyword_validator<spec::error_context_entry> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        auto type_result = validate_protected_object_keywords<spec::error_context_entry>(actual_schema, baseline_schema);
        if (!type_result.has_value()) {
            return type_result;
        }

        if (auto properties_result = validate_object_keyword_matches(actual_schema, baseline_schema, "properties");
            !properties_result.has_value()) {
            return properties_result;
        }

        return validate_object_keyword_matches(actual_schema, baseline_schema, "required");
    }
};

template <>
struct protected_schema_keyword_validator<spec::error_context> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        auto type_result = validate_protected_object_keywords<spec::error_context>(actual_schema, baseline_schema);
        if (!type_result.has_value()) {
            return type_result;
        }

        return validate_object_keyword_matches(actual_schema, baseline_schema, "items");
    }
};

template <>
struct protected_schema_keyword_validator<spec::error> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        auto type_result = validate_protected_object_keywords<spec::error>(actual_schema, baseline_schema);
        if (!type_result.has_value()) {
            return type_result;
        }

        if (auto properties_result = validate_object_keyword_matches(actual_schema, baseline_schema, "properties");
            !properties_result.has_value()) {
            return properties_result;
        }

        return validate_object_keyword_matches(actual_schema, baseline_schema, "required");
    }
};

template <typename... Specs>
struct protected_schema_keyword_validator<spec::one_of<Specs...>> {
    static expected_void validate(JsonVariantConst actual_schema, JsonVariantConst baseline_schema) {
        return validate_object_keyword_matches(actual_schema, baseline_schema, "anyOf");
    }
};

template <typename ContributorList>
struct schema_contributor_emitter;

template <>
struct schema_contributor_emitter<type_list<>> {
    static expected_void emit(JsonVariant) {
        return {};
    }
};

template <typename FirstContributor, typename... RestContributors>
struct schema_contributor_emitter<type_list<FirstContributor, RestContributors...>> {
    static expected_void emit(JsonVariant destination) {
        using result_type = std::remove_cv_t<std::remove_reference_t<decltype(FirstContributor::apply(destination))>>;
        static_assert(std::is_same_v<result_type, expected_void>, "schema contributor hook must return json::expected_void");

        auto result = FirstContributor::apply(destination);
        if (!result.has_value()) {
            return result;
        }

        return schema_contributor_emitter<type_list<RestContributors...>>::emit(destination);
    }
};

template <typename Spec>
expected_void apply_spec_schema_contributors(JsonVariant destination) {
    using contributor_list = typename spec_schema_contributors<Spec>::type;

    if constexpr (std::is_same_v<contributor_list, type_list<>>) {
        return {};
    } else {
        JsonDocument baseline_document;
        JsonVariant baseline_schema = baseline_document.to<JsonVariant>();
        auto baseline_result = base_schema_emitter<Spec>::emit(baseline_schema);
        if (!baseline_result.has_value()) {
            return baseline_result;
        }

        auto contributor_result = schema_contributor_emitter<contributor_list>::emit(destination);
        if (!contributor_result.has_value()) {
            return contributor_result;
        }

        return protected_schema_keyword_validator<Spec>::validate(destination.as<JsonVariantConst>(), baseline_schema.as<JsonVariantConst>());
    }
}

template <typename InnerSpec, typename Codec>
expected_void apply_codec_schema_enrichment(JsonVariant destination) {
    if constexpr (!has_wrapped_codec_schema_enrichment_v<Codec, InnerSpec>) {
        return {};
    } else {
        using result_type = std::remove_cv_t<std::remove_reference_t<decltype(
            Codec::template enrich_schema<InnerSpec>(destination))>>;
        static_assert(std::is_same_v<result_type, expected_void>, "spec::with_codec enrich_schema hook must return json::expected_void");

        JsonDocument baseline_document;
        JsonVariant baseline_schema = baseline_document.to<JsonVariant>();
        auto baseline_result = schema_emitter<InnerSpec>::emit(baseline_schema);
        if (!baseline_result.has_value()) {
            return baseline_result;
        }

        auto enrichment_result = Codec::template enrich_schema<InnerSpec>(destination);
        if (!enrichment_result.has_value()) {
            return enrichment_result;
        }

        return protected_schema_keyword_validator<InnerSpec>::validate(
            destination.as<JsonVariantConst>(),
            baseline_schema.as<JsonVariantConst>());
    }
}

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
void emit_not_empty_keyword(JsonObject schema) {
    using not_empty_option = typename spec_descriptor<Spec>::not_empty_option;

    if constexpr (!std::is_void_v<not_empty_option>) {
        if constexpr (spec_descriptor<Spec>::kind == node_kind::string) {
            schema["minLength"] = 1;
        } else if constexpr (spec_descriptor<Spec>::kind == node_kind::array_of) {
            schema["minItems"] = std::max<size_t>(1U, schema["minItems"] | 0U);
        }
    }
}

template <typename Spec>
struct base_schema_emitter {
    static expected_void emit(JsonVariant) {
        static_assert(always_false_v<Spec>, "schema generation is not implemented for this spec node");
        return std::unexpected(error{error_code::not_implemented, {}, "schema generation is not implemented"});
    }
};

template <typename Spec>
struct schema_emitter {
    static expected_void emit(JsonVariant destination) {
        auto base_result = base_schema_emitter<Spec>::emit(destination);
        if (!base_result.has_value()) {
            return base_result;
        }

        return apply_spec_schema_contributors<Spec>(destination);
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
    auto result = schema_emitter<FieldSpec>::emit(property_schema);
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
struct base_schema_emitter<spec::null<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "null";
        return {};
    }
};

template <typename... Options>
struct base_schema_emitter<spec::boolean<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "boolean";
        return {};
    }
};

template <typename... Options>
struct base_schema_emitter<spec::integer<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "integer";
        emit_min_max_value_keywords<spec::integer<Options...>>(schema);
        return {};
    }
};

template <typename... Options>
struct base_schema_emitter<spec::number<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "number";
        emit_min_max_value_keywords<spec::number<Options...>>(schema);
        return {};
    }
};

template <typename... Options>
struct base_schema_emitter<spec::string<Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "string";
        emit_not_empty_keyword<spec::string<Options...>>(schema);
        emit_pattern_keyword<spec::string<Options...>>(schema);
        return {};
    }
};

template <typename EnumOrCodec, typename... Options>
struct base_schema_emitter<spec::enum_string<EnumOrCodec, Options...>> {
    static expected_void emit(JsonVariant destination) {
        static_assert(
            has_schema_enum_mapping<EnumOrCodec>::value,
            "enum_string requires either json::traits::enum_strings specialization or a lumalink::json::enum_codec-derived type with values for schema generation");

        using mapping_provider = detail::enum_mapping_provider<EnumOrCodec>;

        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "string";

        JsonArray enum_values = schema["enum"].template to<JsonArray>();
        for (const auto& entry : mapping_provider::values()) {
            enum_values.add(std::string(entry.token));
        }

        return {};
    }
};

template <typename... Options>
struct base_schema_emitter<spec::any<Options...>> {
    static expected_void emit(JsonVariant destination) {
        destination.to<JsonObject>();
        return {};
    }
};

template <typename InnerSpec, typename Codec>
struct base_schema_emitter<spec::with_codec<InnerSpec, Codec>> {
    static expected_void emit(JsonVariant destination) {
        if constexpr (has_wrapped_codec_schema_v<Codec, InnerSpec>) {
            using result_type = std::remove_cv_t<std::remove_reference_t<decltype(
                Codec::template emit_schema<InnerSpec>(destination))>>;
            static_assert(
                std::is_same_v<result_type, expected_void>,
                "spec::with_codec schema hook must return json::expected_void");

            return Codec::template emit_schema<InnerSpec>(destination);
        } else {
            return schema_emitter<InnerSpec>::emit(destination);
        }
    }
};

template <typename InnerSpec, typename Codec>
struct schema_emitter<spec::with_codec<InnerSpec, Codec>> {
    static expected_void emit(JsonVariant destination) {
        auto base_result = base_schema_emitter<spec::with_codec<InnerSpec, Codec>>::emit(destination);
        if (!base_result.has_value()) {
            return base_result;
        }

        if constexpr (has_wrapped_codec_schema_v<Codec, InnerSpec>) {
            return {};
        } else {
            return apply_codec_schema_enrichment<InnerSpec, Codec>(destination);
        }
    }
};

template <fixed_string Key, typename ValueSpec, typename... Options>
struct base_schema_emitter<spec::field<Key, ValueSpec, Options...>> {
    static expected_void emit(JsonVariant destination) {
        return schema_emitter<ValueSpec>::emit(destination);
    }
};

template <typename... Fields>
struct base_schema_emitter<spec::object<Fields...>> {
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
struct base_schema_emitter<spec::array_of<ElementSpec, Options...>> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "array";
        emit_min_max_elements_keywords<spec::array_of<ElementSpec, Options...>>(schema);
        emit_not_empty_keyword<spec::array_of<ElementSpec, Options...>>(schema);

        JsonVariant item_schema = schema["items"].template to<JsonVariant>();
        return schema_emitter<ElementSpec>::emit(item_schema);
    }
};

template <typename... Specs>
struct base_schema_emitter<spec::tuple<Specs...>> {
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
struct base_schema_emitter<spec::optional<ValueSpec>> {
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

template <>
struct base_schema_emitter<spec::error_context_entry> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "object";

        JsonObject properties = schema["properties"].template to<JsonObject>();

        JsonVariant kind_schema = properties["kind"].template to<JsonVariant>();
        auto kind_result = schema_emitter<spec::enum_string<node_kind>>::emit(kind_schema);
        if (!kind_result.has_value()) {
            return kind_result;
        }

        JsonVariant logical_name_schema = properties["logical_name"].template to<JsonVariant>();
        auto logical_name_result = schema_emitter<spec::string<>>::emit(logical_name_schema);
        if (!logical_name_result.has_value()) {
            return logical_name_result;
        }

        JsonVariant field_key_schema = properties["field_key"].template to<JsonVariant>();
        auto field_key_result = schema_emitter<spec::string<>>::emit(field_key_schema);
        if (!field_key_result.has_value()) {
            return field_key_result;
        }

        JsonVariant index_schema = properties["index"].template to<JsonVariant>();
        auto index_result = schema_emitter<spec::integer<>>::emit(index_schema);
        if (!index_result.has_value()) {
            return index_result;
        }

        JsonArray required = schema["required"].template to<JsonArray>();
        required.add("kind");
        return {};
    }
};

template <>
struct base_schema_emitter<spec::error_context> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "array";

        JsonVariant item_schema = schema["items"].template to<JsonVariant>();
        return schema_emitter<spec::error_context_entry>::emit(item_schema);
    }
};

template <>
struct base_schema_emitter<spec::error> {
    static expected_void emit(JsonVariant destination) {
        JsonObject schema = destination.to<JsonObject>();
        schema["type"] = "object";

        JsonObject properties = schema["properties"].template to<JsonObject>();

        JsonVariant code_schema = properties["code"].template to<JsonVariant>();
        auto code_result = schema_emitter<spec::enum_string<error_code>>::emit(code_schema);
        if (!code_result.has_value()) {
            return code_result;
        }

        JsonVariant context_schema = properties["context"].template to<JsonVariant>();
        auto context_result = schema_emitter<spec::error_context>::emit(context_schema);
        if (!context_result.has_value()) {
            return context_result;
        }

        JsonVariant message_schema = properties["message"].template to<JsonVariant>();
        auto message_result = schema_emitter<spec::string<>>::emit(message_schema);
        if (!message_result.has_value()) {
            return message_result;
        }

        JsonVariant backend_status_schema = properties["backend_status"].template to<JsonVariant>();
        auto backend_status_result = schema_emitter<spec::integer<>>::emit(backend_status_schema);
        if (!backend_status_result.has_value()) {
            return backend_status_result;
        }

        JsonArray required = schema["required"].template to<JsonArray>();
        required.add("code");
        required.add("context");
        return {};
    }
};

template <typename... Specs>
struct base_schema_emitter<spec::one_of<Specs...>> {
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