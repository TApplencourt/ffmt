// Negative test harness. Each test file expands NEG_EXPECT_REJECTED(fmt, ...)
// once. The Makefile then compiles the file three times:
//   -DFFMT_BUFFER_PATH=0  → ffmt specifiers path; must fail to compile
//   -DFFMT_BUFFER_PATH=1  → ffmt buffer path;     must fail to compile
//   -DNEG_CHECK_FMT       → fmt::format;          must fail to compile
//
// The {fmt} build (our output reference, checked at compile time in C++20)
// is what stops us from inventing restrictions ffmt rejects but {fmt}
// accepts — i.e. "wrong" negative tests.
#pragma once

#if defined(NEG_CHECK_FMT)
  #include <fmt/format.h>
  #define NEG_EXPECT_REJECTED(fmt_str, ...) \
    (void)::fmt::format(fmt_str __VA_OPT__(,) __VA_ARGS__)
#else
  #include <ffmt/base.hpp>
  #define NEG_EXPECT_REJECTED(fmt_str, ...) \
    FFMT_PRINT(fmt_str __VA_OPT__(,) __VA_ARGS__)
#endif
