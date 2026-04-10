#pragma once

#include <string_view>

namespace lumalink::json {

template <typename Derived, typename Enum>
struct enum_codec {
    using enum_type = Enum;
};

} // namespace lumalink::json

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

namespace lumalink::json::detail {

template <typename EnumOrCodec, typename = void>
struct enum_mapping_provider {
    using enum_type = EnumOrCodec;

    [[nodiscard]] static constexpr const auto& values() noexcept {
        return traits::enum_strings<EnumOrCodec>::values;
    }
};

template <typename EnumCodec>
struct enum_mapping_provider<
    EnumCodec,
    std::void_t<typename EnumCodec::enum_type, decltype(EnumCodec::values)>> {
    using enum_type = typename EnumCodec::enum_type;

    [[nodiscard]] static constexpr const auto& values() noexcept {
        return EnumCodec::values;
    }
};

template <typename EnumOrCodec>
using enum_mapping_enum_t = typename enum_mapping_provider<EnumOrCodec>::enum_type;

} // namespace lumalink::json::detail
