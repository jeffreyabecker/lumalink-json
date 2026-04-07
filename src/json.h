#pragma once

#include <expected>
#include <string_view>

namespace lumalink::json {

enum class error_code : unsigned char {
    ok = 0,
    not_implemented,
};

struct error {
    error_code code{error_code::ok};
    std::string_view message{};
};

template <typename T>
using expected = std::expected<T, error>;

using expected_void = std::expected<void, error>;

constexpr expected_void not_implemented() noexcept {
    return std::unexpected(error{error_code::not_implemented, "not implemented"});
}

} // namespace lumalink::json
