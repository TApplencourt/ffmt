// Negative test: too few arguments when one of them has a custom formatter.
// Rejected by validate_format with a readable diagnostic, before formatter
// expansion (which used to fail deep inside <tuple>).
#include "negative.hpp"
struct pt { int x, y; };
#if defined(NEG_CHECK_FMT)
template <> struct fmt::formatter<pt> : fmt::formatter<int> {
  auto format(pt p, fmt::format_context &ctx) const { return fmt::formatter<int>::format(p.x, ctx); }
};
#elif !defined(NEG_SYNTAX_CHECK)
template <> struct ffmt::formatter<pt> {
  static constexpr auto format(pt p) {
    return ffmt::formatted<ffmt::detail::fixed_string{"({}, {})"}, int, int>{{p.x, p.y}};
  }
};
#endif
int main() {
  NEG_EXPECT_REJECTED("{} {}\n", pt{1, 2});
}
