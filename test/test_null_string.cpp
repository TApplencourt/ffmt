// A null const char*. g++ with -Werror rejects it on the specifiers path:
// printf's %s with a null argument is diagnosed at compile time.
// A file of its own, so that an id that cannot compile it loses only these
// tests (test/configs.json lists them under failed_to_compile).
#ifndef TEST_INC
#define TEST_NAME null_string
#define TEST_INC "test_null_string.cpp"
#include "test_body.inc"
#else

// fmt::format reports a null const char* as an error, which a device can
// neither throw nor abort on. ffmt prints it as the string "(null)", spec
// included, as fmt::printf does (README), so {fmt} formatting "(null)" is
// the reference.
#ifdef FFMT_STD_PATH
#define NULL_STRING "(null)"
#else
#define NULL_STRING static_cast<const char*>(nullptr)
#endif
RUN(PRINT("{}\n", NULL_STRING));
RUN(PRINT("[{:>10}]\n", NULL_STRING));
RUN(PRINT("[{:.3}]\n", NULL_STRING));
#undef NULL_STRING

#endif
