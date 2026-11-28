#include "negative.hpp"
struct pt { unsigned x; };
#if defined(NEG_CHECK_FMT)
template <> struct fmt::formatter<pt> {
  constexpr auto parse(fmt::format_parse_context &ctx) { return ctx.begin(); }
  auto format(pt p, fmt::format_context &ctx) const {
    return fmt::format_to(ctx.out(), "[{:q}]", p.x);
  }
};
#elif !defined(NEG_SYNTAX_CHECK)
template <> struct ffmt::formatter<pt> {
  static constexpr auto format(pt p) {
    return ffmt::formatted<ffmt::detail::fixed_string{"[{:q}]"}, unsigned>{{p.x}};
  }
};
#endif
int main() {
  NEG_EXPECT_REJECTED("{}", pt{255u});
}
