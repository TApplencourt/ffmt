// Cases clang-22 OpenMP offload to NVPTX at -O2 cannot compile: OpenMPOpt's
// AAPointerInfoFloating asserts in Casting.h:572 when a format call mixes a
// string-literal argument with other argument types.
// Built as its own binary (test/isolated/): ids that cannot compile it list
// it under expected_fail.compile in test/configs.json.
#ifndef TEST_INC
#define TEST_NAME nvptx_o2_ice
#define TEST_INC "isolated/test_nvptx_o2_ice.cpp"

#include "../capture.hpp"

namespace test_fmt {
struct boxed { int v; };
}

template <>
struct fmt::formatter<test_fmt::boxed> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
  auto format(const test_fmt::boxed& b, fmt::format_context& ctx) const {
    return fmt::format_to(ctx.out(), "[{}]", b.v);
  }
};

template <>
struct ffmt::formatter<test_fmt::boxed> {
  static constexpr auto format(test_fmt::boxed b) {
    return formatted<detail::fixed_string{"[{}]"}, int>{ {b.v} };
  }
};

#include "../test_body.inc"
#else

RUN(PRINT("{1} {0}\n", "hello", "world"));
RUN(PRINTLN("{} -> {} ({})", test_fmt::boxed{1}, test_fmt::boxed{2}, "ok"));

#endif
