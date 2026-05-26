// Negative test harness. Each test file expands NEG_EXPECT_REJECTED(fmt, ...)
// once. The Makefile then compiles the file three times:
//   -DFFMT_BUFFER_PATH=0  → ffmt specifiers path; must fail to compile
//   -DFFMT_BUFFER_PATH=1  → ffmt buffer path;     must fail to compile
//   -DNEG_CHECK_STD       → std::format;          must fail to compile
//
// The std::format build is what stops us from inventing restrictions ffmt
// rejects but std::format accepts — i.e. "wrong" negative tests.
#pragma once

#if defined(NEG_CHECK_STD)
  #include <format>
  #define NEG_EXPECT_REJECTED(fmt, ...) \
    (void)std::format(fmt __VA_OPT__(,) __VA_ARGS__)
#else
  #include <ffmt/base.hpp>
  #define NEG_EXPECT_REJECTED(fmt, ...) \
    FFMT_PRINT(fmt __VA_OPT__(,) __VA_ARGS__)
#endif
