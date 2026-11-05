// Negative test harness. Each test file expands NEG_EXPECT_REJECTED(fmt, ...)
// once. Makefile.id compiles every file three ways on every id and records
// each exit status (which statuses are expected is judged elsewhere):
//   FFMT_SYNTAX_FLAGS (-DNEG_SYNTAX_CHECK) → the call made a no-op; must
//                                            compile, so a typo cannot pass
//                                            as a "rejection"
//   FFMT_FMT_FLAGS    (-DNEG_CHECK_FMT)    → fmt::format; must fail
//   the id's own flags                     → ffmt; must fail
//
// The {fmt} build (our output reference, checked at compile time in C++20)
// is what stops us from inventing restrictions ffmt rejects but {fmt}
// accepts — i.e. "wrong" negative tests.
#pragma once

#if defined(NEG_SYNTAX_CHECK)
  // The file itself must be valid C++: only the format call may fail.
  #define NEG_EXPECT_REJECTED(fmt_str, ...) \
    ((void)fmt_str, [](auto &&...) {}(__VA_ARGS__))
#elif defined(NEG_CHECK_FMT)
  #include <fmt/format.h>
  #define NEG_EXPECT_REJECTED(fmt_str, ...) \
    (void)::fmt::format(fmt_str __VA_OPT__(,) __VA_ARGS__)
#else
  #include <ffmt/base.hpp>
  #define NEG_EXPECT_REJECTED(fmt_str, ...) \
    FFMT_PRINT(fmt_str __VA_OPT__(,) __VA_ARGS__)
#endif
