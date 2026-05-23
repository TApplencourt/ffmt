#ifndef TEST_INC
#define TEST_NAME strings
#define TEST_INC "test_strings.cpp"
#include "test_select_body.inc"
#else

// Char
RUN(PRINT("{}\n", 'A'));
RUN(PRINT("{:d}\n", true));
RUN(PRINT("{}\n", 'Z'));
RUN(PRINT("{:d}\n", 'Z'));
RUN(PRINT("{:c}\n", 65));

// Bool/string as %s — see "spirv-o0" / "ptx-clang-o0" in capture.hpp.
#define _S (FMT_SPIRV_O0 || FMT_PTX_CLANG_O0)
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{}\n", false));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{}\n", true));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:>10}\n", true));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:<10}\n", false));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:s}\n", true));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:s}\n", false));

// println
RUN(PRINTLN("hello println"));
SKIP_IF(FMT_PTX_CLANG_O0, "ptx-clang-o0",
        PRINTLN("{} + {} = {}", 1, 2, 3));
RUN(PRINTLN("{:08x}", 255u));

// Dynamic char
RUN({
  volatile char c = 'Q';
  PRINT("{}\n", static_cast<char>(c));
});

// Width on char
RUN(PRINT("{:<10c}\n", 'B'));
RUN(PRINT("{:>10c}\n", 'C'));

// String %s tests — same gate.
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{}\n", "hello world"));
// OMP target / CUDA: skip — host-side `const char*` isn't accessible
// on device and would need `map(to: env[:len])` or a malloc_shared /
// cudaMemcpy equivalent. Not worth the carve-out for what's a runtime-
// transfer test, not a formatter test.
#if !defined(_OPENMP) && !defined(__CUDACC__)
{
  const char *env = "cpu-char-* copied";
#if defined(FMT_STD_PATH) || !(defined(SYCL_LANGUAGE_VERSION) || FMT_SYCL_COMPILER_ACPP)
  SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{}\n", env));
#else
  size_t len = std::strlen(env) + 1;
  char *shared = ::sycl::malloc_shared<char>(len, q);
  std::memcpy(shared, env, len);
  SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{}\n", shared));
  ::sycl::free(shared, q);
#endif
}
#endif
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:<20s}\n", "hello"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:>20s}\n", "hello"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:.5s}\n", "hello world"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:.0s}\n", "hello"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:.100s}\n", "hello"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:>10.5s}\n", "hello world"));
SKIP_IF(_S, "spirv-o0|ptx-clang-o0", PRINT("{:<10.3s}\n", "hello"));
#undef _S


#endif
