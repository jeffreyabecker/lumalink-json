#pragma once

#include <string_view>

namespace lumalink::json::traits {

template <typename Enum>
struct enum_mapping_entry {
    std::string_view token{};
    Enum value{};
};

template <typename Enum>
struct enum_strings;

template <typename Model>
struct object_fields;

} // namespace lumalink::json::traits
