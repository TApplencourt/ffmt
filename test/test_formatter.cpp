#ifndef TEST_INC
#define TEST_NAME formatter
#define TEST_INC "test_formatter.cpp"

#include "capture.hpp" // pulls in <fmt/format.h> and ffmt/base.hpp

#if defined(SYCL_LANGUAGE_VERSION) || FFMT_COMPILER_ACPP
#include "sycl_std_formatters.hpp"
#endif

// Non-SYCL custom type with a formatter — exercises the customization point
// in coverage builds (which don't include <sycl/sycl.hpp>) as well as device
// builds. Specializations must be visible before the test body is included.
namespace test_fmt {
struct point { int x; int y; };
struct boxed { int v; };
struct swapped { int x; int y; };          // formatter uses positional indices
struct segment { point a; point b; };      // formatter values are custom types
}

template <>
struct fmt::formatter<test_fmt::point> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
  auto format(const test_fmt::point& p, fmt::format_context& ctx) const {
    return fmt::format_to(ctx.out(), "({}, {})", p.x, p.y);
  }
};
template <>
struct fmt::formatter<test_fmt::boxed> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
  auto format(const test_fmt::boxed& b, fmt::format_context& ctx) const {
    return fmt::format_to(ctx.out(), "[{}]", b.v);
  }
};

template <>
struct fmt::formatter<test_fmt::swapped> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
  auto format(const test_fmt::swapped& p, fmt::format_context& ctx) const {
    return fmt::format_to(ctx.out(), "({1}, {0})", p.x, p.y);
  }
};
template <>
struct fmt::formatter<test_fmt::segment> {
  constexpr auto parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
  auto format(const test_fmt::segment& g, fmt::format_context& ctx) const {
    return fmt::format_to(ctx.out(), "{}->{}", g.a, g.b);
  }
};

template <>
struct ffmt::formatter<test_fmt::point> {
  static constexpr auto format(test_fmt::point p) {
    return formatted<detail::fixed_string{"({}, {})"}, int, int>{ {p.x, p.y} };
  }
};
template <>
struct ffmt::formatter<test_fmt::boxed> {
  static constexpr auto format(test_fmt::boxed b) {
    return formatted<detail::fixed_string{"[{}]"}, int>{ {b.v} };
  }
};

template <>
struct ffmt::formatter<test_fmt::swapped> {
  static constexpr auto format(test_fmt::swapped p) {
    return formatted<detail::fixed_string{"({1}, {0})"}, int, int>{ {p.x, p.y} };
  }
};
template <>
struct ffmt::formatter<test_fmt::segment> {
  static constexpr auto format(test_fmt::segment g) {
    return formatted<detail::fixed_string{"{}->{}"}, test_fmt::point, test_fmt::point>{ {g.a, g.b} };
  }
};

#include "test_select_body.inc"
#else

// Custom non-SYCL formatter — every PTX-clang-O0 test below mishandles
// the custom-formatter splicer output. Left runnable on other backends.
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINTLN("p = {}", test_fmt::point{1, 2}));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINTLN("b = {}", test_fmt::boxed{42}));

// Mixed primitive + custom (auto-indexed) — exercises the splicer with
// non-trivial Fmt2 expansion and the runtime args-tuple flatten.
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0",
        PRINTLN("step {} of {}: p={}", 3, 100, test_fmt::point{4, 8}));
// clang-22 NVPTX -O2 ICE — see FFMT_PTX_CLANG_O2_ICE comment in capture.hpp.
#if FFMT_PTX_CLANG_O2_ICE
SKIP_IF(1, "o2-ptx-ice", PRINT(""));
#else
SKIP_IF((FFMT_SPIRV_O0 || FFMT_PTX_CLANG_O0), "spirv-o0|ptx-clang-o0",
        PRINTLN("{} -> {} ({})", test_fmt::boxed{1}, test_fmt::boxed{2}, "ok"));
#endif

// All-primitive path on the same entry point — should bypass expansion
RUN(PRINTLN("plain {} works too", 42));

// PRINT (no newline) variant
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0", PRINT("[{}]\n", test_fmt::point{0, 0}));

#if defined(SYCL_LANGUAGE_VERSION) || FFMT_COMPILER_ACPP
// SYCL types — only available in real device builds
RUN(PRINTLN("range = {}", sycl::range<1>{4}));
RUN(PRINTLN("range = {}", sycl::range<2>{4, 8}));
RUN(PRINTLN("range = {}", sycl::range<3>{4, 8, 16}));

RUN(PRINTLN("id = {}", sycl::id<1>{2}));
RUN(PRINTLN("id = {}", sycl::id<2>{2, 5}));
RUN(PRINTLN("id = {}", sycl::id<3>{1, 2, 3}));

RUN(PRINTLN("step {} of {}: range={}", 3, 100, sycl::range<3>{4, 8, 16}));

RUN(PRINT("[{}]\n", sycl::id<2>{0, 0}));
#endif

// Indices are renumbered when a formatter is spliced in, so positional
// indices work in formatter strings and in the outer string (the specifiers
// path printed "(2, 1) (2, 1)" for the first case and rejected the others).
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0",
        PRINTLN("{} {}", test_fmt::swapped{1, 2}, test_fmt::swapped{3, 4}));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0",
        PRINTLN("{1} then {0}", test_fmt::point{1, 2}, test_fmt::boxed{3}));
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0",
        PRINTLN("{0} {0} {1}", test_fmt::boxed{7}, 5));
// Nested: the formatter's values are themselves custom types.
SKIP_IF(FFMT_PTX_CLANG_O0, "ptx-clang-o0",
        PRINTLN("seg {} n={}", test_fmt::segment{{1, 2}, {3, 4}}, 9));

#endif
