// Negative test: a format spec on an argument with a custom formatter. The
// formatter has no spec parser ({fmt}'s parse() returns at once, so {fmt}
// rejects it too). The buffer path used to ignore the spec silently.
#include "negative.hpp"
struct pt { int x, y; };
#if defined(NEG_CHECK_FMT)
template <> struct fmt::formatter<pt> {
  constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }
  auto format(pt p, fmt::format_context &ctx) const { return fmt::format_to(ctx.out(), "{}", p.x); }
};
#elif !defined(NEG_SYNTAX_CHECK)
template <> struct ffmt::formatter<pt> {
  static constexpr auto format(pt p) {
    return ffmt::formatted<ffmt::detail::fixed_string{"({}, {})"}, int, int>{{p.x, p.y}};
  }
};
#endif
int main() {
  NEG_EXPECT_REJECTED("{:>10}\n", pt{1, 2});
}
