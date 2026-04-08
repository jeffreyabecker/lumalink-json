#pragma once

#include <type_traits>

namespace lumalink::json::compile_fail_support {

template <typename Tag>
struct intentionally_invalid_case {
    static_assert(!std::is_same_v<Tag, Tag>, "intentional compile-fail harness case");
};

} // namespace lumalink::json::compile_fail_support
