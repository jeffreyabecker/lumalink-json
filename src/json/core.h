#pragma once

#include <ArduinoJson.h>

#include <array>
#include <cstddef>
#include <expected>
#include <string_view>

namespace lumalink::json {

template <size_t N>
struct fixed_string {
    char value[N]{};

    constexpr fixed_string(const char (&input)[N]) noexcept {
        for (size_t index = 0; index < N; ++index) {
            value[index] = input[index];
        }
    }

    [[nodiscard]] constexpr std::string_view view() const noexcept {
        return std::string_view{value, N - 1};
    }
};

template <size_t N>
fixed_string(const char (&)[N]) -> fixed_string<N>;

enum class error_code : unsigned char {
    ok = 0,
    missing_field,
    unexpected_type,
    array_size_mismatch,
    validation_failed,
    value_out_of_range,
    pattern_mismatch,
    enum_string_unknown,
    enum_value_unmapped,
    empty_input,
    incomplete_input,
    invalid_input,
    no_memory,
    too_deep,
    not_implemented,
};

enum class node_kind : unsigned char {
    unknown = 0,
    null_value,
    boolean,
    integer,
    number,
    string,
    enum_string,
    any,
    field,
    object,
    array_of,
    tuple,
    optional,
    one_of,
};

enum class context_policy : unsigned char {
    full = 0,
    last,
    none,
};

struct error_context_entry {
    node_kind kind{node_kind::unknown};
    std::string_view logical_name{};
    std::string_view field_key{};
    size_t index{0};
    bool has_index{false};
};

inline constexpr size_t error_context_capacity = 8;

struct error_context {
    std::array<error_context_entry, error_context_capacity> entries{};
    size_t size{0};
};

struct error {
    error_code code{error_code::ok};
    error_context context{};
    std::string_view message{};
    int backend_status{0};
};

template <typename T>
using expected = std::expected<T, error>;

using expected_void = std::expected<void, error>;

struct decode_options {
    context_policy context{context_policy::full};
    size_t nesting_limit{0};
};

struct encode_options {
    context_policy context{context_policy::full};
};

struct decode_state {
    context_policy context{context_policy::full};
};

struct encode_state {
    context_policy context{context_policy::full};
};

constexpr error with_context(
    error value,
    const error_context_entry entry,
    const context_policy policy = context_policy::full) noexcept {
    if (policy == context_policy::none) {
        return value;
    }

    if (policy == context_policy::last) {
        value.context.entries[0] = entry;
        value.context.size = 1;
        return value;
    }

    if (value.context.size < value.context.entries.size()) {
        value.context.entries[value.context.size] = entry;
        ++value.context.size;
        return value;
    }

    for (size_t index = 1; index < value.context.entries.size(); ++index) {
        value.context.entries[index - 1] = value.context.entries[index];
    }

    value.context.entries[value.context.entries.size() - 1] = entry;
    return value;
}

constexpr expected_void not_implemented() noexcept {
    return std::unexpected(error{error_code::not_implemented, {}, "not implemented"});
}

static_assert(static_cast<int>(error_code::ok) == 0, "error_code::ok must be zero");

} // namespace lumalink::json
